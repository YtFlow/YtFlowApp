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

    std::string_view TrimSpaces(std::string_view input)
    {
        auto const lpos = input.find_first_not_of(' ');
        if (lpos != std::string_view::npos)
        {
            input.remove_prefix(lpos);
        }
        auto const rpos = input.find_last_not_of(' ');
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
}
