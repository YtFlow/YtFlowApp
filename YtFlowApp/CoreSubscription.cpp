#include "pch.h"

#include "CoreProxyImpl.h"
#include "CoreSubscription.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProxyInput
    {
        std::string name;
        nlohmann::json::binary_t proxy;
        uint16_t proxy_version;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProxyInput, name, proxy, proxy_version)
    std::vector<ProxyInput> DecodeSip008(std::string_view data);
    std::vector<ProxyInput> DecodeSurgeProxyList(std::string_view data);

    std::string_view TrimSpaces(std::string_view input)
    {
        auto const lpos = input.find_first_not_of(" \t\r\n");
        if (lpos != std::string_view::npos)
        {
            input.remove_prefix(lpos);
        }
        auto const rpos = input.find_last_not_of(" \t\r\n");
        if (rpos != std::string_view::npos)
        {
            input = input.substr(0, rpos + 1);
        }
        return input;
    }
    DecodedSubscriptionUserInfo DecodeSubscriptionUserInfoFromResponseHeaderValue(std::string_view resValue)
    {
        DecodedSubscriptionUserInfo ret;

        for (auto const kvIt : std::views::split(resValue, ';'))
        {
            std::string_view const kvStr(kvIt.begin(), kvIt.end());
            auto const eqPos = kvStr.find('=');
            if (eqPos == std::string_view::npos)
            {
                continue;
            }

            std::string_view k(kvStr.data(), eqPos), v(kvStr.data() + eqPos + 1, kvStr.size() - eqPos - 1);
            k = TrimSpaces(k);
            v = TrimSpaces(v);
            if (k == "expire")
            {
                errno = 0;
                auto const num = std::strtoul(v.data(), nullptr, 10);
                if (errno != 0)
                {
                    continue;
                }
                auto time = static_cast<std::time_t>(static_cast<uint32_t>(num));
                std::tm ctime;
                if (gmtime_s(&ctime, &time) != 0)
                {
                    continue;
                }
                std::stringstream ss;
                // ISO 8601 date + time
                ss << std::put_time(&ctime, "%F") << 'T' << std::put_time(&ctime, "%T");
                ret.expires_at = {ss.str()};
                continue;
            }
            std::optional<uint64_t> *numSlot{nullptr};
            if (k == "upload")
            {
                numSlot = &ret.upload_bytes_used;
            }
            else if (k == "download")
            {
                numSlot = &ret.download_bytes_used;
            }
            else if (k == "total")
            {
                numSlot = &ret.bytes_total;
            }
            else
            {
                continue;
            }

            errno = 0;
            auto const num = std::strtoull(v.data(), nullptr, 10);
            if (errno != 0)
            {
                continue;
            }
            *numSlot = {num};
        }

        return ret;
    }

    char const *ConvertSubscriptionFormatToStatic(char const *input)
    {
        if (input == nullptr)
        {
            throw std::invalid_argument("Empty input for subscription format");
        }
        if (strcmp(input, "sip008") == 0)
        {
            return SIP008_LITERAL;
        }
        if (strcmp(input, "surge_proxy_list") == 0)
        {
            return SURGE_PROXY_LIST_LITERAL;
        }
        throw std::invalid_argument(std::string("Unknown subscription format: ") + std::string(input));
    }
    std::optional<std::vector<uint8_t>> DecodeSubscriptionProxies(std::string_view data, char const *&decodedFormat)
    {
        if (decodedFormat == nullptr || strcmp(decodedFormat, SIP008_LITERAL) == 0)
        {
            auto sip008Res = DecodeSip008(data);
            if (!sip008Res.empty())
            {
                decodedFormat = SIP008_LITERAL;
                return {nlohmann::json::to_cbor(std::move(sip008Res))};
            }
        }
        if (decodedFormat == nullptr || strcmp(decodedFormat, SURGE_PROXY_LIST_LITERAL) == 0)
        {
            auto surgeRes = DecodeSurgeProxyList(data);
            if (!surgeRes.empty())
            {
                decodedFormat = SURGE_PROXY_LIST_LITERAL;
                return {nlohmann::json::to_cbor(std::move(surgeRes))};
            }
        }
        decodedFormat = nullptr;
        return std::nullopt;
    }

    std::vector<ProxyInput> DecodeSip008(std::string_view data)
    {
        auto doc = nlohmann::json::parse(data, nullptr, false);
        if (doc.type() == nlohmann::json::value_t::discarded)
        {
            return {};
        }
        if (doc.type() == nlohmann::json::value_t::array)
        {
            doc = {{"version", 1}, {"servers", std::move(doc)}};
        }
        if (doc.value("version", 0) != 1)
        {
            return {};
        }
        std::vector<ProxyInput> ret;
        for (auto &&proxyDoc : doc.value("servers", std::vector<nlohmann::json>()))
        {
            std::string remarks = proxyDoc.value("remarks", "New SIP008 proxy"), server = proxyDoc.value("server", ""),
                        password = proxyDoc.value("password", ""),
                        method = proxyDoc.value("method", "chacha20-ietf-poly1305"),
                        plugin = proxyDoc.value("plugin", ""), pluginOpts = proxyDoc.value("plugin_opts", "");
            uint16_t port = proxyDoc.value("server_port", 8388);

            if (server.empty())
            {
                continue;
            }

            std::string_view tcpOut = "$out.tcp", udpOut = "$out.udp";
            std::vector<ProxyPlugin> proxyPlugins;

            // obfs
            ProxyPlugin obfsPlugin{.name = "o"};
            auto const pluginDecodeRes =
                Sip002Decoder::DecodeSip003Plugin(plugin, pluginOpts, server, obfsPlugin, tcpOut);
            if (pluginDecodeRes == PluginDecodeResult::Success)
            {
                proxyPlugins.emplace_back(std::move(obfsPlugin));
                tcpOut = "o.tcp";
            }
            else if (pluginDecodeRes == PluginDecodeResult::Fail)
            {
                continue;
            }

            // redir
            ProxyPlugin redirPlugin{
                .name = "r",
                .plugin = std::string(REDIR_PLUGIN_NAME),
                .param = nlohmann::json::to_cbor(
                    {{"dest", {{"host", server}, {"port", port}}}, {"tcp_next", tcpOut}, {"udp_next", udpOut}}),
            };
            proxyPlugins.emplace_back(std::move(redirPlugin));
            tcpOut = "r.tcp";
            udpOut = "r.udp";

            // protocol
            ProxyPlugin protocolPlugin{
                .name = "p",
                .plugin = std::string(SS_PLUGIN_NAME),
                .param = nlohmann::json::to_cbor(
                    {{"method", std::move(method)},
                     {"password", nlohmann::json::binary_t(std::vector<uint8_t>(password.begin(), password.end()))},
                     {"tcp_next", tcpOut},
                     {"udp_next", udpOut}})};
            proxyPlugins.emplace_back(std::move(protocolPlugin));
            tcpOut = "p.tcp";
            udpOut = "p.udp";

            ret.emplace_back(
                ProxyInput{.name = std::move(remarks),
                           .proxy = nlohmann::json::to_cbor(DynOutboundV1Proxy{.tcp_entry = std::string(tcpOut),
                                                                               .udp_entry = {std::string(udpOut)},
                                                                               .plugins = std::move(proxyPlugins)})});
        }
        return ret;
    }

    std::vector<ProxyInput> DecodeSurgeProxyList(std::string_view data)
    {
        using namespace std::literals::string_view_literals;

        std::vector<ProxyInput> ret;
        for (auto &&lineIt : std::views::split(data, '\n'))
        {
            std::string_view const line(lineIt.begin(), lineIt.end());
            auto const lineEqPos = line.find('=');
            if (lineEqPos == std::string_view::npos)
            {
                continue;
            }

            auto const name = TrimSpaces(line.substr(0, lineEqPos));
            auto segView = std::views::split(line.substr(lineEqPos + 1), ',');
            auto segIt = segView.begin();
            if (segIt == segView.end())
            {
                continue;
            }
            auto const protocol = TrimSpaces(std::string_view((*segIt).begin(), (*segIt).end()));
            if (protocol.empty())
            {
                continue;
            }
            if (++segIt == segView.end())
            {
                continue;
            }
            auto const server = TrimSpaces(std::string_view((*segIt).begin(), (*segIt).end()));
            if (++segIt == segView.end())
            {
                continue;
            }
            auto const portStr = TrimSpaces(std::string_view((*segIt).begin(), (*segIt).end()));
            errno = 0;
            auto const port = static_cast<uint16_t>(strtol(portStr.data(), nullptr, 10));
            if (errno != 0)
            {
                continue;
            }
            ++segIt;
            std::string_view username, password;
            if (std::ranges::any_of(std::array{"http"sv, "https"sv, "socks5"sv, "socks5-tls"sv},
                                    [protocol](auto const p) { return protocol == p; }) &&
                segIt != segView.end())
            {
                username = TrimSpaces(std::string_view((*segIt).begin(), (*segIt).end()));
                if (username.find('=') != std::string_view::npos)
                {
                    username = ""sv;
                }
                else if (++segIt != segView.end())
                {
                    password = TrimSpaces(std::string_view((*segIt).begin(), (*segIt).end()));
                }
                if (password.find('=') != std::string_view::npos)
                {
                    password = ""sv;
                }
            }
            std::map<std::string_view, std::string_view> params;
            while (segIt != segView.end())
            {
                auto const param = std::string_view((*segIt).begin(), (*segIt).end());
                auto const paramEqPos = param.find('=');
                if (paramEqPos == std::string_view::npos)
                {
                    continue;
                }
                auto const paramName = TrimSpaces(param.substr(0, paramEqPos));
                auto const paramValue = TrimSpaces(param.substr(paramEqPos + 1));
                if (paramName.empty() || paramValue.empty())
                {
                    continue;
                }
                params.emplace(paramName, paramValue);
                ++segIt;
            }
            auto pullParam = [&params](std::string_view const name) -> std::optional<std::string_view> {
                auto const it = params.find(name);
                if (it == params.end())
                {
                    return std::nullopt;
                }
                auto const value = it->second;
                params.erase(it); // All params must be checked exactly once
                return std::make_optional(value);
            };
            pullParam("no-error-alert");

            std::string_view tcpOut = "$out.tcp", udpOut = "$out.udp";
            std::vector<ProxyPlugin> proxyPlugins;

            // tls
            if (auto const tlsParam = pullParam("tls"sv);
                (tlsParam.has_value() && tlsParam.value() == "true"sv) ||
                std::ranges::any_of(std::array{"https"sv, "trojan"sv, "socks5-tls"sv},
                                    [protocol](auto const p) { return protocol == p; }))
            {
                ProxyPlugin tlsPlugin{.name = "t", .plugin = std::string(TLS_PLUGIN_NAME)};
                nlohmann::json tlsPluginParam{{"next", tcpOut}};
                if (pullParam("skip-cert-verify"sv).value_or(""sv) == "true")
                {
                    tlsPluginParam["skip_cert_check"] = true;
                }
                auto const sniParam = pullParam("sni"sv);
                if (sniParam.has_value() && sniParam.value() != "off"sv) // TODO: turn off SNI?
                {
                    tlsPluginParam["sni"] = std::string(sniParam.value());
                }

                tlsPlugin.param = nlohmann::json::to_cbor(tlsPluginParam);
                proxyPlugins.emplace_back(std::move(tlsPlugin));
                tcpOut = "t.tcp";
            }

            // obfs
            auto const wsParam = pullParam("ws"sv);
            auto const obfsParam = pullParam("obfs"sv);
            ProxyPlugin obfsPlugin{.name = "o"};
            if (wsParam.has_value() && wsParam.value() == "true"sv)
            {
                obfsPlugin.plugin = std::string(WS_PLUGIN_NAME);
                nlohmann::json wsPluginParam{{"next", tcpOut}, {"host", server}, {"headers", {}}};
                auto const wsPathParam = pullParam("ws-path"sv);
                if (wsPathParam.has_value())
                {
                    wsPluginParam["path"] = std::string(wsPathParam.value());
                }
                auto const wsHeadersParam = pullParam("ws-headers"sv);
                if (wsHeadersParam.has_value())
                {
                    for (auto const &header : std::views::split(wsHeadersParam.value(), '|'))
                    {
                        auto const headerStr = std::string_view(header.begin(), header.end());
                        auto const headerColonPos = headerStr.find(':');
                        if (headerColonPos == std::string_view::npos)
                        {
                            continue;
                        }
                        auto const headerName = TrimSpaces(headerStr.substr(0, headerColonPos));
                        auto const headerValue = TrimSpaces(headerStr.substr(headerColonPos + 1));
                        if (headerName.empty())
                        {
                            continue;
                        }
                        wsPluginParam["headers"][headerName] = headerValue;
                    }
                }
                obfsPlugin.param = nlohmann::json::to_cbor(wsPluginParam);

                proxyPlugins.emplace_back(std::move(obfsPlugin));
                tcpOut = "o.tcp";
            }
            else if (obfsParam.has_value() && obfsParam.value() == "http"sv)
            {
                obfsPlugin.plugin = std::string(HTTP_OBFS_PLUGIN_NAME);
                nlohmann::json httpParam{{"next", tcpOut}, {"host", server}, {"path", "/"}};
                if (auto const httpHostParam = pullParam("obfs-host"sv); httpHostParam.has_value())
                {
                    httpParam["host"] = std::string(httpHostParam.value());
                }
                if (auto const httpPathParam = pullParam("obfs-uri"sv); httpPathParam.has_value())
                {
                    httpParam["path"] = std::string(httpPathParam.value());
                }
                obfsPlugin.param = nlohmann::json::to_cbor(httpParam);

                proxyPlugins.emplace_back(std::move(obfsPlugin));
                tcpOut = "o.tcp";
            }
            else if (obfsParam.has_value() && obfsParam.value() == "tls"sv)
            {
                obfsPlugin.plugin = std::string(TLS_OBFS_PLUGIN_NAME);
                nlohmann::json tlsObfsParam{{"next", tcpOut}, {"host", server}};
                if (auto const tlsHostParam = pullParam("obfs-host"sv); tlsHostParam.has_value())
                {
                    tlsObfsParam["host"] = std::string(tlsHostParam.value());
                }
                obfsPlugin.param = nlohmann::json::to_cbor(tlsObfsParam);

                proxyPlugins.emplace_back(std::move(obfsPlugin));
                tcpOut = "o.tcp";
            }

            // redirect
            ProxyPlugin redirPlugin{
                .name = "r",
                .plugin = std::string(REDIR_PLUGIN_NAME),
                .param = nlohmann::json::to_cbor(
                    {{"dest", {{"host", server}, {"port", port}}}, {"tcp_next", tcpOut}, {"udp_next", udpOut}}),
            };
            proxyPlugins.emplace_back(std::move(redirPlugin));
            tcpOut = "r.tcp";
            udpOut = "r.udp";

            // protocol
            bool enableUdp = false;
            ProxyPlugin protocolPlugin{.name = "p"};
            if (std::ranges::any_of(std::array{"http"sv, "https"sv, "socks5"sv, "socks5-tls"sv},
                                    [protocol](auto const p) { return protocol == p; }))
            {
                nlohmann::json protocolParam{{"next", tcpOut}, {"tcp_next", tcpOut}};
                if (!username.empty())
                {
                    protocolParam["user"] = nlohmann::json::binary_t(
                        std::vector<uint8_t>(username.data(), username.data() + username.size()));
                }
                if (!password.empty())
                {
                    protocolParam["pass"] = nlohmann::json::binary_t(
                        std::vector<uint8_t>(password.data(), password.data() + password.size()));
                }
                if (protocol.starts_with("socks5"sv))
                {
                    protocolPlugin.plugin = std::string(SOCKS5_PLUGIN_NAME);
                    protocolParam["udp_next"] = udpOut;
                }
                else if (protocol.starts_with("http"sv))
                {
                    protocolPlugin.plugin = std::string(HTTP_PROXY_PLUGIN_NAME);
                }
                protocolPlugin.param = nlohmann::json::to_cbor(protocolParam);
            }
            else if (protocol == "ss"sv)
            {
                protocolPlugin.plugin = std::string(SS_PLUGIN_NAME);
                auto const method = pullParam("encrypt-method"sv), ssPassword = pullParam("password"sv);
                if (!method.has_value() || !ssPassword.has_value())
                {
                    continue;
                }
                if (pullParam("udp-relay").value_or(""sv) == "true"sv)
                {
                    enableUdp = true;
                }
                protocolPlugin.param = nlohmann::json::to_cbor(
                    {{"method", method.value()},
                     {"password",
                      nlohmann::json::binary_t(std::vector<uint8_t>(
                          ssPassword.value().data(), ssPassword.value().data() + ssPassword.value().size()))},
                     {"tcp_next", tcpOut},
                     {"udp_next", udpOut}});
            }
            else if (protocol == "trojan"sv)
            {
                protocolPlugin.plugin = std::string(TROJAN_PLUGIN_NAME);
                auto const trojanPassword = pullParam("password"sv);
                if (!trojanPassword.has_value())
                {
                    continue;
                }
                protocolPlugin.param = nlohmann::json::to_cbor(
                    {{"password", nlohmann::json::binary_t(std::vector<uint8_t>(trojanPassword.value().data(),
                                                                                trojanPassword.value().data() +
                                                                                    trojanPassword.value().size()))},
                     {"tls_next", tcpOut}});
            }
            else if (protocol == "vmess"sv)
            {
                protocolPlugin.plugin = std::string(VMESS_PLUGIN_NAME);
                auto const vmessUsername = pullParam("username"sv), method = pullParam("encrypt-method"sv),
                           vmessAead = pullParam("vmess-aead"sv);
                if (!vmessUsername.has_value())
                {
                    continue;
                }
                protocolPlugin.param =
                    nlohmann::json::to_cbor({{"user_id", vmessUsername.value()},
                                             {"security", method.value_or("auto"sv)},
                                             {"alter_id", vmessAead.value_or(""sv) == "true"sv ? 0 : 1},
                                             {"tcp_next", tcpOut}});
            }
            else
            {
                continue;
            }
            proxyPlugins.emplace_back(std::move(protocolPlugin));
            tcpOut = "p.tcp";
            if (enableUdp)
            {
                udpOut = "p.udp";
            }

            if (!params.empty())
            {
                // Still have unrecognized params
                continue;
            }

            ret.emplace_back(
                ProxyInput{.name = std::string(name),
                           .proxy = nlohmann::json::to_cbor(DynOutboundV1Proxy{.tcp_entry = std::string(tcpOut),
                                                                               .udp_entry = {std::string(udpOut)},
                                                                               .plugins = std::move(proxyPlugins)})});
        }
        return ret;
    }
}
