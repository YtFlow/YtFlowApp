#include "pch.h"
#include "CoreProxy.h"

#include <optional>
#include <string>

#include "CoreProxyImpl.h"
#include "base64.h"

namespace winrt::YtFlowApp::implementation
{
    std::string UriEscape(std::string_view const input)
    {
        std::vector<char> buf((input.size() + 1) * 3);
        uriEscapeExA(input.data(), input.data() + input.size(), buf.data(), URI_TRUE, URI_FALSE);
        return std::string(buf.data());
    }
    std::string UriUnescape(std::string buf)
    {
        if (!buf.empty())
        {
            auto const endPos = uriUnescapeInPlaceExA(buf.data(), URI_TRUE, URI_BR_DONT_TOUCH);
            buf.resize(endPos - buf.data());
        }
        return buf;
    }
    std::optional<std::pair<std::string, std::string>> SplitUserPassFromUserinfo(std::string_view rawUserinfo)
    {
        auto const colonPos = rawUserinfo.find(':');
        if (colonPos == std::string_view::npos)
        {
            return std::nullopt;
        }
        std::string user = UriUnescape(std::string(rawUserinfo.data(), colonPos));
        std::string pass =
            UriUnescape(std::string(rawUserinfo.data() + colonPos + 1, rawUserinfo.size() - colonPos - 1));
        return {std::make_pair(std::move(user), std::move(pass))};
    }
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertUriToProxy(ParsedUri const &uri)
    {
        if (uri.scheme == SS_SCHEME)
        {
            if (uri.userInfo.empty())
            {
                return DecodeUriToProxy(uri, LegacyShadowsocksDecoder{});
            }
            return DecodeUriToProxy(uri, Sip002Decoder{});
        }
        if (uri.scheme == TROJAN_SCHEME)
        {
            return DecodeUriToProxy(uri, TrojanDecoder{});
        }
        if (uri.scheme == SOCKS5_SCHEME || uri.scheme == SOCKS5H_SCHEME)
        {
            return DecodeUriToProxy(uri, Socks5Decoder{});
        }
        if (uri.scheme == HTTP_SCHEME && uri.hostText != "t.me")
        {
            // TODO: t.me proxy
            return DecodeUriToProxy(uri, HttpDecoder{});
        }
        if (uri.scheme == VMESS_SCHEME)
        {
            return DecodeUriToProxy(uri, V2raynDecoder{});
        }
        return std::nullopt;
    }

    std::string Sip002Decoder::DecodeName(ParsedUri const &uri)
    {
        return UriUnescape(std::string(uri.fragment));
    }
    PluginDecodeResult Sip002Decoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin,
                                                     std::string_view tcpNext, std::string_view udpNext) const
    {
        plugin.plugin = std::string(SS_PLUGIN_NAME);
        std::string userinfo;
        try
        {
            userinfo = base64_decode(UriUnescape(std::string(uri.userInfo)));
        }
        catch (...)
        {
            return PluginDecodeResult::Fail;
        }
        auto const colonPos = userinfo.find(':');
        if (colonPos == std::string::npos)
        {
            return PluginDecodeResult::Fail;
        }
        plugin.param = nlohmann::json::to_cbor(
            {{"method", std::string_view(userinfo.data(), colonPos)},
             {"password", nlohmann::json::binary_t(
                              std::vector<uint8_t>(userinfo.data() + colonPos + 1, userinfo.data() + userinfo.size()))},
             {"tcp_next", tcpNext},
             {"udp_next", udpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult Sip002Decoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        auto const portOpt = ParsePort(uri.portText);
        if (!portOpt.has_value())
        {
            return PluginDecodeResult::Fail;
        }
        port = *portOpt;
        host = uri.hostText;
        return PluginDecodeResult::Success;
    }

    PluginDecodeResult Sip002Decoder::DecodeSip003Plugin(std::string_view pluginName, std::string_view pluginOpts,
                                                         std::string_view defaultHost, ProxyPlugin &plugin,
                                                         std::string_view tcpNext)
    {
        if (pluginName == "")
        {
            return PluginDecodeResult::Ignore;
        }
        if (pluginName != "obfs-local")
        {
            return PluginDecodeResult::Fail;
        }
        std::map<std::string_view, std::string_view> obfsMap;
        for (auto const &kv : std::views::split(pluginOpts, ';'))
        {
            std::string_view kvStr(kv.begin(), kv.end());
            auto const eqPos = kvStr.find('=');
            if (eqPos == std::string_view::npos)
            {
                return PluginDecodeResult::Fail;
            }
            obfsMap[std::string_view(kvStr.data(), eqPos)] =
                std::string_view(kvStr.data() + eqPos + 1, kvStr.size() - eqPos - 1);
        }
        std::string_view host = obfsMap["obfs-host"];
        if (host.empty())
        {
            host = defaultHost;
        }
        if (obfsMap["obfs"] == "http")
        {
            std::string_view path = "/";
            if (!obfsMap["obfs-uri"].empty())
            {
                path = obfsMap["obfs-uri"];
            }
            plugin.plugin = HTTP_OBFS_PLUGIN_NAME;
            plugin.param = nlohmann::json::to_cbor({{"host", host}, {"path", path}, {"next", tcpNext}});
        }
        else if (obfsMap["obfs"] == "tls")
        {
            plugin.plugin = TLS_OBFS_PLUGIN_NAME;
            plugin.param = nlohmann::json::to_cbor({{"host", host}, {"next", tcpNext}});
        }
        else
        {
            return PluginDecodeResult::Fail;
        }
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult Sip002Decoder::DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                                 std::string_view) const
    {
        std::string empty{};
        auto const &param = uri.GetQueryValue("plugin").value_or(std::ref(empty)).get();
        auto const semiPos = param.find(';');
        if (semiPos == std::string::npos)
        {
            return DecodeSip003Plugin("", std::string_view(param.data(), param.size()), uri.hostText, plugin, tcpNext);
        }
        return DecodeSip003Plugin(std::string_view(param.data(), semiPos),
                                  std::string_view(param.data() + semiPos + 1, param.size() - semiPos - 1),
                                  uri.hostText, plugin, tcpNext);
    }
    PluginDecodeResult Sip002Decoder::DecodeTls(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        // TODO: tls
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult Sip002Decoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Success;
    }

    std::string LegacyShadowsocksDecoder::DecodeName(ParsedUri const &uri)
    {
        return Sip002Decoder{}.DecodeName(uri);
    }
    bool LegacyShadowsocksDecoder::decodeUserinfoParts(ParsedUri const &uri)
    {
        if (m_userinfoPartsDecoded)
        {
            return true;
        }

        std::string userinfo;
        try
        {
            userinfo = base64_decode(UriUnescape(std::string(uri.hostText)));
        }
        catch (...)
        {
            return false;
        }
        auto const firstColon = userinfo.find(':');
        if (firstColon == std::string::npos)
        {
            return false;
        }
        auto const firstAt = userinfo.find('@', firstColon);
        if (firstAt == std::string::npos)
        {
            return false;
        }
        auto const secondColon = userinfo.find(':', firstAt);
        if (secondColon == std::string::npos)
        {
            return false;
        }

        m_userinfoParts =
            std::make_tuple(std::string(userinfo.data(), firstColon),
                            std::vector<uint8_t>(userinfo.data() + firstColon + 1, userinfo.data() + firstAt),
                            std::string(userinfo.begin() + firstAt + 1, userinfo.begin() + secondColon),
                            std::string(userinfo.begin() + secondColon + 1, userinfo.end()));
        m_userinfoPartsDecoded = true;

        return true;
    }
    PluginDecodeResult LegacyShadowsocksDecoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin,
                                                                std::string_view tcpNext, std::string_view udpNext)
    {
        if (!decodeUserinfoParts(uri))
        {
            return PluginDecodeResult::Fail;
        }
        plugin.plugin = std::string(SS_PLUGIN_NAME);
        auto const &secondUserinfoPart = std::get<1>(m_userinfoParts);
        plugin.param = nlohmann::json::to_cbor(
            {{"method", std::get<0>(m_userinfoParts)},
             {"password", nlohmann::json::binary_t(std::vector(secondUserinfoPart.data(),
                                                               secondUserinfoPart.data() + secondUserinfoPart.size()))},
             {"tcp_next", tcpNext},
             {"udp_next", udpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult LegacyShadowsocksDecoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        if (!decodeUserinfoParts(uri))
        {
            return PluginDecodeResult::Fail;
        }
        auto const portOpt = ParsePort(std::get<3>(m_userinfoParts));
        if (!portOpt.has_value())
        {
            return PluginDecodeResult::Fail;
        }
        port = *portOpt;
        host = std::get<2>(m_userinfoParts);
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult LegacyShadowsocksDecoder::DecodeObfs(ParsedUri const &, ProxyPlugin &, std::string_view,
                                                            std::string_view)
    {
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult LegacyShadowsocksDecoder::DecodeTls(ParsedUri const &, ProxyPlugin &, std::string_view,
                                                           std::string_view)
    {
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult LegacyShadowsocksDecoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Success;
    }

    std::string TrojanDecoder::DecodeName(ParsedUri const &uri)
    {
        return UriUnescape(std::string(uri.fragment));
    }
    PluginDecodeResult TrojanDecoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin,
                                                     std::string_view tcpNext, std::string_view) const
    {
        plugin.plugin = std::string(TROJAN_PLUGIN_NAME);
        std::string userinfo(UriUnescape(std::string(uri.userInfo)));
        plugin.param = nlohmann::json::to_cbor({{"password", nlohmann::json::binary_t(std::vector<uint8_t>(
                                                                 userinfo.data(), userinfo.data() + userinfo.size()))},
                                                {"tls_next", tcpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult TrojanDecoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        if (uri.portText.empty())
        {
            port = 443;
        }
        else
        {
            auto const portOpt = ParsePort(uri.portText);
            if (!portOpt.has_value())
            {
                return PluginDecodeResult::Fail;
            }
            port = *portOpt;
        }
        host = uri.hostText;
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult TrojanDecoder::DecodeObfs(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        // TODO: trojan-go ws
        return PluginDecodeResult::Ignore;
    }
    nlohmann::json ParseAlpn(std::string_view alpnStr)
    {
        nlohmann::json alpns;
        for (auto const &s : std::views::split(alpnStr, ','))
        {
            if (s.empty())
            {
                continue;
            }
            alpns.push_back(std::string_view(s.begin(), s.end()));
        }
        return alpns;
    }
    PluginDecodeResult TrojanDecoder::DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                                std::string_view) const
    {
        nlohmann::json tlsParam{{"next", tcpNext}};
        auto const allowInsecureParam = uri.GetQueryValue("allowInsecure"), peerParam = uri.GetQueryValue("peer"),
                   alpnParam = uri.GetQueryValue("alpn");
        std::string empty{};
        if (allowInsecureParam.value_or(std::ref(empty)).get() == "1")
        {
            tlsParam["skip_cert_check"] = true;
        }
        if (peerParam.has_value())
        {
            tlsParam["sni"] = peerParam.value().get();
        }
        if (alpnParam.has_value())
        {
            auto const &alpnStr = alpnParam.value().get();
            tlsParam["alpn"] = ParseAlpn(alpnStr);
        }
        plugin.param = nlohmann::json::to_cbor(std::move(tlsParam));

        return PluginDecodeResult::Success;
    }
    PluginDecodeResult TrojanDecoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Ignore;
    }
    std::string Socks5Decoder::DecodeName(ParsedUri const &uri)
    {
        auto const remarks = uri.GetQueryValue("remarks");
        if (remarks.has_value())
        {
            return remarks.value().get();
        }
        return UriUnescape(std::string(uri.fragment));
    }
    PluginDecodeResult Socks5Decoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin,
                                                     std::string_view tcpNext, std::string_view udpNext) const
    {
        plugin.plugin = std::string(SOCKS5_PLUGIN_NAME);
        nlohmann::json userDoc = nullptr, passDoc = nullptr;
        if (!uri.userInfo.empty())
        {
            auto userPassSplit = SplitUserPassFromUserinfo(uri.userInfo);
            if (!userPassSplit.has_value())
            {
                return PluginDecodeResult::Fail;
            }
            auto const &[user, pass] = userPassSplit.value();
            userDoc = nlohmann::json::binary_t(std::vector<uint8_t>(user.data(), user.data() + user.size()));
            passDoc = nlohmann::json::binary_t(std::vector<uint8_t>(pass.data(), pass.data() + pass.size()));
        }
        plugin.param = nlohmann::json::to_cbor(
            {{"user", std::move(userDoc)}, {"pass", std::move(passDoc)}, {"tcp_next", tcpNext}, {"udp_next", udpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult Socks5Decoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        auto const portOpt = ParsePort(uri.portText);
        if (!portOpt.has_value())
        {
            return PluginDecodeResult::Fail;
        }
        port = *portOpt;
        host = uri.hostText;
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult Socks5Decoder::DecodeObfs(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult Socks5Decoder::DecodeTls(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        // TODO: tls
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult Socks5Decoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Ignore;
    }
    std::string HttpDecoder::DecodeName(ParsedUri const &uri)
    {
        return UriUnescape(std::string(uri.fragment));
    }
    PluginDecodeResult HttpDecoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                                   std::string_view) const
    {
        plugin.plugin = std::string(HTTP_PROXY_PLUGIN_NAME);
        nlohmann::json userDoc = nlohmann::json::binary_t(std::vector<uint8_t>()),
                       passDoc = nlohmann::json::binary_t(std::vector<uint8_t>());
        if (!uri.userInfo.empty())
        {
            auto userPassSplit = SplitUserPassFromUserinfo(uri.userInfo);
            if (!userPassSplit.has_value())
            {
                return PluginDecodeResult::Fail;
            }
            auto const &[user, pass] = userPassSplit.value();
            userDoc = nlohmann::json::binary_t(std::vector<uint8_t>(user.data(), user.data() + user.size()));
            passDoc = nlohmann::json::binary_t(std::vector<uint8_t>(pass.data(), pass.data() + pass.size()));
        }
        plugin.param = nlohmann::json::to_cbor(
            {{"user", std::move(userDoc)}, {"pass", std::move(passDoc)}, {"tcp_next", tcpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult HttpDecoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        if (uri.portText.empty())
        {
            port = 80;
            // TODO: HTTPS
        }
        else
        {
            auto const portOpt = ParsePort(uri.portText);
            if (!portOpt.has_value())
            {
                return PluginDecodeResult::Fail;
            }
            port = *portOpt;
        }
        host = uri.hostText;
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult HttpDecoder::DecodeObfs(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult HttpDecoder::DecodeTls(ParsedUri const &, ProxyPlugin &, std::string_view, std::string_view)
    {
        // TODO: https tls
        return PluginDecodeResult::Ignore;
    }
    PluginDecodeResult HttpDecoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Ignore;
    }
    [[nodiscard]] bool V2raynDecoder::decodeJsonDoc(ParsedUri const &uri)
    {
        if (m_jsonDecoded)
        {
            return true;
        }

        try
        {
            std::string b64Text(uri.hostText);
            for (auto const pathSegment : uri.pathSegments)
            {
                b64Text += "/";
                b64Text += pathSegment;
            }
            std::string userinfo = base64_decode(std::move(b64Text));
            m_linkDoc = nlohmann::json::parse(std::move(userinfo));
            if (!m_linkDoc.contains("v") || (m_linkDoc.at("v") != 2 && m_linkDoc.at("v") != "2"))
            {
                return false;
            }
        }
        catch (...)
        {
            return false;
        }

        m_jsonDecoded = true;
        return true;
    }
    std::string V2raynDecoder::DecodeName(ParsedUri const &uri)
    {
        if (!decodeJsonDoc(uri) || !m_linkDoc.contains("ps"))
        {
            return "Erroneous V2RayN VMess Proxy";
        }
        return m_linkDoc.at("ps");
    }
    PluginDecodeResult V2raynDecoder::DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin,
                                                     std::string_view tcpNext, std::string_view)
    {
        if (!decodeJsonDoc(uri))
        {
            return PluginDecodeResult::Fail;
        }
        plugin.plugin = std::string(VMESS_PLUGIN_NAME);
        std::string uid, security;
        uint16_t alterId;
        try
        {
            auto const enableVlessDoc = m_linkDoc.value("enable_vless", nlohmann::json());
            if (enableVlessDoc == true || enableVlessDoc == "true")
            {
                return PluginDecodeResult::Fail;
            }
            uid = m_linkDoc.at("id");
            security = m_linkDoc.value("scy", "auto");
            if (security == "zero")
            {
                return PluginDecodeResult::Fail;
            }
            auto const aidDoc = m_linkDoc.at("aid");
            if (aidDoc.type() == nlohmann::json::value_t::string)
            {
                errno = 0;
                alterId = static_cast<uint16_t>(std::strtol(aidDoc.get<std::string>().data(), nullptr, 10));
                if (errno != 0)
                {
                    return PluginDecodeResult::Fail;
                }
            }
            else if (aidDoc.type() == nlohmann::json::value_t::number_integer ||
                     aidDoc.type() == nlohmann::json::value_t::number_unsigned)
            {
                alterId = aidDoc;
            }
            else
            {
                return PluginDecodeResult::Fail;
            }
        }
        catch (...)
        {
            return PluginDecodeResult::Fail;
        }
        plugin.param = nlohmann::json::to_cbor({{"user_id", std::move(uid)},
                                                {"security", std::move(security)},
                                                {"alter_id", alterId},
                                                {"tcp_next", tcpNext}});
        return PluginDecodeResult::Success;
    }
    PluginDecodeResult V2raynDecoder::DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port)
    {
        if (!decodeJsonDoc(uri))
        {
            return PluginDecodeResult::Fail;
        }
        try
        {
            auto const portDoc = m_linkDoc.at("port");
            if (portDoc.type() == nlohmann::json::value_t::string)
            {
                auto const portOpt = ParsePort(portDoc);
                if (!portOpt.has_value())
                {
                    return PluginDecodeResult::Fail;
                }
                port = *portOpt;
            }
            else if (portDoc.type() == nlohmann::json::value_t::number_integer ||
                     portDoc.type() == nlohmann::json::value_t::number_unsigned)
            {
                port = portDoc;
            }
            else
            {
                return PluginDecodeResult::Fail;
            }

            host = m_linkDoc.at("add");
            return PluginDecodeResult::Success;
        }
        catch (...)
        {
            return PluginDecodeResult::Fail;
        }
    }
    PluginDecodeResult V2raynDecoder::DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                                 std::string_view)
    {
        if (!decodeJsonDoc(uri))
        {
            return PluginDecodeResult::Fail;
        }
        try
        {
            auto const typeDoc = m_linkDoc.value("type", "");
            if (typeDoc != "none" && typeDoc != "vmess" && m_linkDoc.at("type") != "")
            {
                return PluginDecodeResult::Fail;
            }
            if (m_linkDoc.at("net") == "tcp")
            {
                return PluginDecodeResult::Ignore;
            }
            if (m_linkDoc.at("net") == "ws")
            {
                plugin.plugin = WS_PLUGIN_NAME;
                std::string host = m_linkDoc.value("host", ""), path = m_linkDoc.value("path", "/");
                if (host.empty())
                {
                    host = m_linkDoc.at("add");
                }
                plugin.param = nlohmann::json::to_cbor({{"host", std::move(host)},
                                                        {"path", std::move(path)},
                                                        {"headers", std::map<std::string, std::string>()},
                                                        {"next", tcpNext}});
                return PluginDecodeResult::Success;
            }
            return PluginDecodeResult::Fail;
        }
        catch (...)
        {
            return PluginDecodeResult::Fail;
        }
    }
    PluginDecodeResult V2raynDecoder::DecodeTls(ParsedUri const &uri, ProxyPlugin &tlsPlugin, std::string_view tcpNext,
                                                std::string_view)
    {
        if (!decodeJsonDoc(uri))
        {
            return PluginDecodeResult::Fail;
        }
        try
        {
            if (m_linkDoc.value("tls", "") != "tls")
            {
                return PluginDecodeResult::Ignore;
            }
            auto const enableXtlsDoc = m_linkDoc.value("enable_xtls", nlohmann::json());
            if (enableXtlsDoc == true || enableXtlsDoc == "true")
            {
                return PluginDecodeResult::Fail;
            }
            // TODO: fingerprint
            nlohmann::json tlsParamDoc =
                               {// TODO: allow insecure or not?
                                {"skip_cert_check", true},
                                {"next", tcpNext}},
                           alpnDoc = nullptr;
            auto const isWs = m_linkDoc.value("net", "tcp") == "ws";
            auto const rawAlpn = m_linkDoc.value("alpn", "");
            if (isWs && rawAlpn == "h2,http/1.1")
            {
                // websocket-client is ALPN-aware. Omit alpn to enable h2 probe.
            }
            else
            {
                alpnDoc = ParseAlpn(rawAlpn);
            }
            if (alpnDoc.type() == nlohmann::json::value_t::array && !alpnDoc.empty())
            {
                tlsParamDoc["alpn"] = std::move(alpnDoc);
            }
            if (m_linkDoc.value("sni", "") != "")
            {
                tlsParamDoc["sni"] = m_linkDoc.at("sni").get<std::string>();
            }
            tlsPlugin.param = nlohmann::json::to_cbor(std::move(tlsParamDoc));
            return PluginDecodeResult::Success;
        }
        catch (...)
        {
            return PluginDecodeResult::Fail;
        }
    }
    PluginDecodeResult V2raynDecoder::DecodeUdp(ParsedUri const &)
    {
        return PluginDecodeResult::Ignore;
    }

    std::optional<uint16_t> ParsePort(std::string_view portText)
    {
        int port{};
        if (auto const [_, ec] = std::from_chars(portText.data(), portText.data() + portText.size(), port);
            ec != std::errc())
        {
            return std::nullopt;
        }
        if (port < 0 || port > 65535)
        {
            return std::nullopt;
        }
        return std::make_optional(static_cast<uint16_t>(port));
    }
    std::map<std::string, std::string> ParseQuery(std::string_view uriQuery)
    {
        UriQueryListA *queryList;
        int itemCount;
        if (uriDissectQueryMallocA(&queryList, &itemCount, uriQuery.data(), uriQuery.data() + uriQuery.size()) !=
            URI_SUCCESS)
        {
            return {};
        }
        std::map<std::string, std::string> ret;
        UriQueryListStructA const *currentQuery = queryList;
        while (currentQuery != nullptr)
        {
            ret[std::string(currentQuery->key)] = std::string(currentQuery->value);
            currentQuery = currentQuery->next;
        }
        uriFreeQueryListA(queryList);
        return ret;
    }
    UriTextRangeA StringViewToUriTextRangeA(std::string_view sv)
    {
        if (sv.empty())
        {
            return UriTextRangeA{.first = nullptr, .afterLast = nullptr};
        }
        return UriTextRangeA{.first = sv.data(), .afterLast = sv.data() + sv.size()};
    }
    std::string_view UriTextRangeAToStringView(UriTextRangeA const &text)
    {
        if (text.first == nullptr)
        {
            return {};
        }
        return std::string_view(text.first, text.afterLast);
    }

    std::optional<std::reference_wrapper<std::string>> ParsedUri::GetQueryValue(std::string const &key) const
    {
        if (!queryMap.contains(key))
        {
            return std::nullopt;
        }
        auto &[value, visited] = queryMap[key];
        visited = true;
        return {std::ref(value)};
    }
    bool ParsedUri::HasUnvisitedQuery() const noexcept
    {
        return std::ranges::find_if(queryMap, [](auto const &kv) { return !kv.second.second; }) != queryMap.end();
    }
    ParsedUri ParsedUri::fromUri(UriUriA const &uri)
    {
        ParsedUri parsedUri;
        parsedUri.scheme = UriTextRangeAToStringView(uri.scheme);
        parsedUri.userInfo = UriTextRangeAToStringView(uri.userInfo);
        parsedUri.hostText = UriTextRangeAToStringView(uri.hostText);
        parsedUri.portText = UriTextRangeAToStringView(uri.portText);
        parsedUri.query = UriTextRangeAToStringView(uri.query);
        parsedUri.fragment = UriTextRangeAToStringView(uri.fragment);

        auto pathPtr = uri.pathHead;
        while (pathPtr != nullptr)
        {
            UriTextRangeA const &pathText = pathPtr->text;
            parsedUri.pathSegments.emplace_back(UriTextRangeAToStringView(pathText));
            pathPtr = pathPtr->next;
        }

        if (!parsedUri.query.empty())
        {
            auto queryMap = ParseQuery(parsedUri.query);
            std::ranges::transform(
                std::move(queryMap), std::inserter(parsedUri.queryMap, parsedUri.queryMap.end()), [](auto &&kv) {
                    return std::make_pair(std::move(kv.first), std::make_pair(std::move(kv.second), false));
                });
        }

        return parsedUri;
    }

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertShareLinkToProxy(std::string const &link)
    {
        UriUriA uri;
        char const *errPos;
        if (uriParseSingleUriA(&uri, link.c_str(), &errPos) != 0)
        {
            return std::nullopt;
        }

        auto const parsedUri = ParsedUri::fromUri(uri);
        auto ret = ConvertUriToProxy(parsedUri);
        uriFreeUriMembersA(&uri);
        return ret;
    }
    std::optional<std::string> ConvertProxyToShareLink(std::string_view name, std::span<uint8_t const> proxy)
    {
        DynOutboundV1Proxy proxyObj;
        try
        {
            proxyObj = nlohmann::json::from_cbor(proxy);
        }
        catch (...)
        {
            return std::nullopt;
        }
        std::optional<std::reference_wrapper<ProxyPlugin const>> protocolPlugin, redirectPlugin, transportPlugin,
            tlsPlugin;
        for (auto const &plugin : proxyObj.plugins)
        {
            if (plugin.plugin_version != 0)
            {
                continue;
            }
            // TODO: analyze dependency
            if (plugin.plugin == TLS_PLUGIN_NAME)
            {
                if (tlsPlugin.has_value())
                {
                    return std::nullopt;
                }
                tlsPlugin = std::make_optional(std::cref(plugin));
                continue;
            }
            if (plugin.plugin == HTTP_OBFS_PLUGIN_NAME || plugin.plugin == TLS_OBFS_PLUGIN_NAME ||
                plugin.plugin == WS_PLUGIN_NAME)
            {
                if (transportPlugin.has_value())
                {
                    return std::nullopt;
                }
                transportPlugin = std::make_optional(std::cref(plugin));
                continue;
            }
            if (plugin.plugin == REDIR_PLUGIN_NAME)
            {
                if (redirectPlugin.has_value())
                {
                    return std::nullopt;
                }
                redirectPlugin = std::make_optional(std::cref(plugin));
                continue;
            }
            if (protocolPlugin.has_value())
            {
                return std::nullopt;
            }
            protocolPlugin = std::make_optional(std::cref(plugin));
        }
        if (!protocolPlugin.has_value() || !redirectPlugin.has_value())
        {
            return std::nullopt;
        }
        if (protocolPlugin.value().get().plugin == SS_PLUGIN_NAME)
        {
            if (tlsPlugin.has_value())
            {
                return std::nullopt;
            }
            return ConvertSsToLink(name, protocolPlugin.value(), redirectPlugin.value(), transportPlugin);
        }
        if (protocolPlugin.value().get().plugin == TROJAN_PLUGIN_NAME)
        {
            if (transportPlugin.has_value() || !tlsPlugin.has_value())
            {
                return std::nullopt;
            }
            return ConvertTrojanToLink(name, protocolPlugin.value(), redirectPlugin.value(), tlsPlugin.value());
        }
        if (protocolPlugin.value().get().plugin == SOCKS5_PLUGIN_NAME && !transportPlugin.has_value() &&
            !tlsPlugin.has_value())
        {
            return ConvertSocks5ToLink(name, protocolPlugin.value(), redirectPlugin.value());
        }
        if (protocolPlugin.value().get().plugin == HTTP_PROXY_PLUGIN_NAME && !transportPlugin.has_value() &&
            !tlsPlugin.has_value())
        {
            return ConvertHttpProxyToLink(name, protocolPlugin.value(), redirectPlugin.value());
        }
        if (protocolPlugin.value().get().plugin == VMESS_PLUGIN_NAME)
        {
            return ConvertVMessProxyToLink(name, protocolPlugin.value(), redirectPlugin.value(), transportPlugin,
                                           tlsPlugin);
        }
        return std::nullopt;
    }
    std::optional<std::string> ComposeUri(UriUriA const &uri)
    {
        int charsRequired{}, charsWritten{};
        if (uriToStringCharsRequiredA(&uri, &charsRequired) != URI_SUCCESS)
        {
            return std::nullopt;
        }
        charsRequired++;
        std::vector<char> ret(charsRequired);
        if (uriToStringA(ret.data(), &uri, charsRequired, &charsWritten) != URI_SUCCESS)
        {
            return std::nullopt;
        }
        return std::string(ret.data(), charsWritten - 1);
    }

    std::optional<std::string> ConvertSsToLink(std::string_view name, ProxyPlugin const &protocolPlugin,
                                               ProxyPlugin const &redirectPlugin,
                                               std::optional<std::reference_wrapper<ProxyPlugin const>> obfsPlugin)
    {
        std::string method, host, port, obfsPluginParam;
        nlohmann::json::binary_t password;
        try
        {
            auto const redirParam = nlohmann::json::from_cbor(redirectPlugin.param);
            auto const ssParam = nlohmann::json::from_cbor(protocolPlugin.param);
            method = ssParam.at("method");
            password = ssParam.at("password");
            host = redirParam.at("dest").at("host");
            port = std::to_string(static_cast<int>(redirParam.at("dest").at("port")));
            if (obfsPlugin.has_value())
            {
                auto const &op = obfsPlugin.value().get();
                auto const opDoc = nlohmann::json::from_cbor(op.param);
                if (op.plugin == HTTP_OBFS_PLUGIN_NAME)
                {
                    obfsPluginParam = std::string("obfs-local;obfs=http;obfs-host=") + opDoc.value("host", "") +
                                      ";obfs-uri=" + opDoc.value("path", "/");
                }
                else if (op.plugin == TLS_OBFS_PLUGIN_NAME)
                {
                    obfsPluginParam = std::string("obfs-local;obfs=tls;obfs-host=") + opDoc.value("host", "");
                }
                else
                {
                    return std::nullopt;
                }
            }
        }
        catch (...)
        {
            return std::nullopt;
        }
        auto const userinfo =
            base64_encode(method + ':' + std::string(reinterpret_cast<char *>(password.data()), password.size()));

        auto const escapedName = UriEscape(name);
        std::optional<std::string> querystring;
        UriTextRangeA queryRange{};
        if (!obfsPluginParam.empty())
        {
            UriQueryListA queryList{.key = "plugin", .value = obfsPluginParam.c_str(), .next = nullptr};
            querystring = ComposeQuery(&queryList);
            if (!querystring.has_value())
            {
                return std::nullopt;
            }
            auto const &qs = querystring.value();
            queryRange = {.first = qs.c_str(), .afterLast = qs.c_str() + qs.size()};
        }

        UriUriA uri{.scheme = StringViewToUriTextRangeA(SS_SCHEME),
                    .userInfo = StringViewToUriTextRangeA(userinfo),
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .query = queryRange,
                    .fragment = StringViewToUriTextRangeA(escapedName.data())};
        return ComposeUri(uri);
    }

    std::optional<std::string> ComposeQuery(UriQueryListA const *queryList)
    {
        if (queryList == nullptr)
        {
            return {};
        }
        int charsRequired;
        int qsCharsWritten;
        if (uriComposeQueryCharsRequiredA(queryList, &charsRequired) != URI_SUCCESS)
        {
            return std::nullopt;
        }
        charsRequired++;
        std::vector<char> qs(charsRequired);
        if (uriComposeQueryA(qs.data(), queryList, charsRequired, &qsCharsWritten) != URI_SUCCESS)
        {
            return std::nullopt;
        }
        return {std::string(qs.data(), qsCharsWritten - 1)};
    }

    std::optional<std::string> ConvertTrojanToLink(std::string_view name, ProxyPlugin const &protocolPlugin,
                                                   ProxyPlugin const &redirectPlugin, ProxyPlugin const &tlsPlugin)
    {
        std::string host, port, alpn;
        nlohmann::json::binary_t password;
        std::optional<std::string> peer;
        bool allowInsecure = false;
        try
        {
            auto const redirParam = nlohmann::json::from_cbor(redirectPlugin.param);
            auto const trojanParam = nlohmann::json::from_cbor(protocolPlugin.param);
            auto const tlsParam = nlohmann::json::from_cbor(tlsPlugin.param);
            password = trojanParam.at("password");
            host = redirParam.at("dest").at("host");
            port = std::to_string(static_cast<int>(redirParam.at("dest").at("port")));
            if (tlsParam.contains("sni"))
            {
                peer = std::make_optional(std::string(tlsParam.at("sni")));
            }
            if (tlsParam.value("skip_cert_check", false) == true)
            {
                allowInsecure = true;
            }
            std::vector<std::string> const alpns = tlsParam.value("alpn", std::vector<std::string>());
            for (auto const &a : alpns)
            {
                alpn += a;
                alpn.push_back(',');
            }
            if (!alpn.empty())
            {
                alpn = alpn.substr(0, alpn.size() - 1);
            }
        }
        catch (...)
        {
            return std::nullopt;
        }

        UriQueryListA *queryList = nullptr, peerQuery = {.key = "peer", .value = "", .next = nullptr},
                      allowInsecureQuery = {.key = "allowInsecure", .value = "1", .next = nullptr},
                      alpnQuery = {.key = "alpn", .value = "", .next = nullptr};
        if (peer.has_value())
        {
            peerQuery.next = queryList;
            peerQuery.value = peer.value().c_str();
            queryList = &peerQuery;
        }
        if (allowInsecure)
        {
            allowInsecureQuery.next = queryList;
            queryList = &allowInsecureQuery;
        }
        if (!alpn.empty())
        {
            alpnQuery.next = queryList;
            alpnQuery.value = alpn.c_str();
            queryList = &alpnQuery;
        }
        std::optional<std::string> querystring;
        UriTextRangeA queryRange{};
        if (queryList != nullptr)
        {
            querystring = ComposeQuery(queryList);
            if (!querystring.has_value())
            {
                return std::nullopt;
            }
            auto const &qs = querystring.value();
            queryRange = {.first = qs.c_str(), .afterLast = qs.c_str() + qs.size()};
        }
        auto const escapedName = UriEscape(name);

        UriUriA uri{.scheme = StringViewToUriTextRangeA(TROJAN_SCHEME),
                    .userInfo =
                        UriTextRangeA{.first = reinterpret_cast<char const *>(password.data()),
                                      .afterLast = reinterpret_cast<char const *>(password.data()) + password.size()},
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .query = queryRange,
                    .fragment = StringViewToUriTextRangeA(escapedName.data())};
        return ComposeUri(uri);
    }

    std::optional<std::string> ConvertSocks5ToLink(std::string_view name, ProxyPlugin const &protocolPlugin,
                                                   ProxyPlugin const &redirectPlugin)
    {
        std::string host, port, userinfo;
        try
        {
            auto const redirParam = nlohmann::json::from_cbor(redirectPlugin.param);
            auto const socks5Param = nlohmann::json::from_cbor(protocolPlugin.param);
            if (socks5Param.value<nlohmann::json>("user", nullptr) != nullptr &&
                socks5Param.value<nlohmann::json>("pass", nullptr) != nullptr)
            {
                nlohmann::json::binary_t userBuf = socks5Param.at("user");
                nlohmann::json::binary_t passBuf = socks5Param.at("pass");
                if (!userBuf.empty())
                {
                    userinfo +=
                        UriEscape(std::string_view(reinterpret_cast<char const *>(userBuf.data()),
                                                   reinterpret_cast<char const *>(userBuf.data() + userBuf.size())));
                    userinfo.push_back(':');
                    userinfo +=
                        UriEscape(std::string_view(reinterpret_cast<char const *>(passBuf.data()),
                                                   reinterpret_cast<char const *>(passBuf.data() + passBuf.size())));
                }
            }
            host = redirParam.at("dest").at("host");
            port = std::to_string(static_cast<int>(redirParam.at("dest").at("port")));
        }
        catch (...)
        {
            return std::nullopt;
        }

        auto const escapedName = UriEscape(name);

        UriUriA uri{.scheme = StringViewToUriTextRangeA(SOCKS5_SCHEME),
                    .userInfo = StringViewToUriTextRangeA(userinfo),
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .fragment = StringViewToUriTextRangeA(escapedName.data())};
        return ComposeUri(uri);
    }
    std::optional<std::string> ConvertHttpProxyToLink(std::string_view name, ProxyPlugin const &protocolPlugin,
                                                      ProxyPlugin const &redirectPlugin)
    {
        std::string host, port, userinfo;
        try
        {
            auto const redirParam = nlohmann::json::from_cbor(redirectPlugin.param);
            auto const httpProxyParam = nlohmann::json::from_cbor(protocolPlugin.param);
            if (httpProxyParam.value<nlohmann::json>("user", nullptr) != nullptr &&
                httpProxyParam.value<nlohmann::json>("pass", nullptr) != nullptr)
            {
                nlohmann::json::binary_t userBuf = httpProxyParam.at("user");
                nlohmann::json::binary_t passBuf = httpProxyParam.at("pass");
                if (!userBuf.empty())
                {
                    userinfo +=
                        UriEscape(std::string_view(reinterpret_cast<char const *>(userBuf.data()),
                                                   reinterpret_cast<char const *>(userBuf.data() + userBuf.size())));
                    userinfo.push_back(':');
                    userinfo +=
                        UriEscape(std::string_view(reinterpret_cast<char const *>(passBuf.data()),
                                                   reinterpret_cast<char const *>(passBuf.data() + passBuf.size())));
                }
            }
            host = redirParam.at("dest").at("host");
            port = std::to_string(static_cast<int>(redirParam.at("dest").at("port")));
        }
        catch (...)
        {
            return std::nullopt;
        }

        auto const escapedName = UriEscape(name);

        UriUriA uri{.scheme = StringViewToUriTextRangeA(HTTP_SCHEME),
                    .userInfo = StringViewToUriTextRangeA(userinfo),
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .fragment = StringViewToUriTextRangeA(escapedName.data())};
        return ComposeUri(uri);
    }
    std::optional<std::string> ConvertVMessProxyToLink(std::string_view, ProxyPlugin const &, ProxyPlugin const &,
                                                       std::optional<std::reference_wrapper<ProxyPlugin const>>,
                                                       std::optional<std::reference_wrapper<ProxyPlugin const>>)
    {
        // TODO: 写不动了。你们 VMess 的分享链接格式真的群魔乱舞。
        return std::nullopt;
    }
}
