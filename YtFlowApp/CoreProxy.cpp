#include "pch.h"
#include "CoreProxy.h"

#include <optional>
#include <string>

#include "CoreProxyImpl.h"
#include "base64.h"

namespace winrt::YtFlowApp::implementation
{
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertUriToProxy(ParsedUri const &uri)
    {
        if (uri.scheme == SS_SCHEME)
        {
            if (uri.userInfo.empty())
            {
                return ConvertLegacySsToProxy(uri);
            }
            return ConvertSip002ToProxy(uri);
        }
        if (uri.scheme == TROJAN_SCHEME)
        {
            return ConvertTrojanUriToProxy(uri);
        }
        return std::nullopt;
    }
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertSip002ToProxy(ParsedUri const &uri)
    {
        std::string frag(uri.fragment);
        uriUnescapeInPlaceExA(frag.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        std::string proxyName(
            frag); // proxyName may contain a \0 in the middle, which truncates the string across FFI boundary.
        if (proxyName.empty())
        {
            proxyName = "New Proxy";
        }
        ProxyPlugin ssPlugin{.name = "p", .plugin = std::string(SS_PLUGIN_NAME)};
        ProxyPlugin redirPlugin{.name = "r", .plugin = std::string(REDIR_PLUGIN_NAME)};

        std::string userinfo(uri.userInfo);
        uriUnescapeInPlaceExA(userinfo.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        try
        {
            userinfo = base64_decode(userinfo);
        }
        catch (...)
        {
            return std::nullopt;
        }
        auto const colonPos = userinfo.find(':');
        if (colonPos == std::string::npos)
        {
            return std::nullopt;
        }
        ssPlugin.param = nlohmann::json::to_cbor(
            {{"method", std::string_view(userinfo.data(), colonPos)},
             {"password", nlohmann::json::binary_t(
                              std::vector<uint8_t>(userinfo.data() + colonPos + 1, userinfo.data() + userinfo.size()))},
             {"tcp_next", "r.tcp"},
             {"udp_next", "r.udp"}});
        auto const port = ParsePort(uri.portText);
        if (!port.has_value())
        {
            return std::nullopt;
        }

        redirPlugin.param = nlohmann::json::to_cbor(
            {{"dest", {{"host", uri.hostText}, {"port", *port}}}, {"tcp_next", "$out.tcp"}, {"udp_next", "$out.udp"}});

        DynOutboundV1Proxy proxy{
            .tcp_entry = "p.tcp", .udp_entry = "p.udp", .plugins = {std::move(ssPlugin), std::move(redirPlugin)}};
        return std::make_pair(std::move(proxyName), nlohmann::json::to_cbor(proxy));
    }

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertLegacySsToProxy(ParsedUri const &uri)
    {
        std::string proxyName;
        if (uri.fragment.empty())
        {
            proxyName = "New Proxy";
        }
        else
        {
            std::string fragment(uri.fragment);
            uriUnescapeInPlaceExA(fragment.data(), URI_TRUE, URI_BR_DONT_TOUCH);
            proxyName = std::move(fragment);
            // proxyName may contain a \0 in the middle, which truncates the string across FFI boundary.
        }
        std::string userinfo(uri.hostText);
        uriUnescapeInPlaceExA(userinfo.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        try
        {
            userinfo = base64_decode(std::move(userinfo));
        }
        catch (...)
        {
            return std::nullopt;
        }
        auto const firstColon = userinfo.find(':');
        if (firstColon == std::string::npos)
        {
            return std::nullopt;
        }
        auto const firstAt = userinfo.find('@', firstColon);
        if (firstAt == std::string::npos)
        {
            return std::nullopt;
        }
        auto const secondColon = userinfo.find(':', firstAt);
        if (secondColon == std::string::npos)
        {
            return std::nullopt;
        }

        auto const portText = std::string_view(userinfo.begin() + secondColon + 1, userinfo.end());
        auto const port = ParsePort(portText);
        if (!port.has_value())
        {
            return std::nullopt;
        }

        ProxyPlugin ssPlugin{.name = "p", .plugin = std::string(SS_PLUGIN_NAME)};
        ProxyPlugin redirPlugin{.name = "r", .plugin = std::string(REDIR_PLUGIN_NAME)};
        ssPlugin.param =
            nlohmann::json::to_cbor({{"method", std::string_view(userinfo.data(), firstColon)},
                                     {"password", nlohmann::json::binary_t(std::vector<uint8_t>(
                                                      userinfo.data() + firstColon + 1, userinfo.data() + firstAt))},
                                     {"tcp_next", "r.tcp"},
                                     {"udp_next", "r.udp"}});
        redirPlugin.param = nlohmann::json::to_cbor(
            {{"dest",
              {{"host", std::string_view(userinfo.begin() + firstAt + 1, userinfo.begin() + secondColon)},
               {"port", *port}}},
             {"tcp_next", "$out.tcp"},
             {"udp_next", "$out.udp"}});

        DynOutboundV1Proxy proxy{
            .tcp_entry = "p.tcp", .udp_entry = "p.udp", .plugins = {std::move(ssPlugin), std::move(redirPlugin)}};
        return std::make_pair(std::move(proxyName), nlohmann::json::to_cbor(proxy));
    }

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertTrojanUriToProxy(ParsedUri const &uri)
    {
        ProxyPlugin trojanPlugin{.name = "p", .plugin = std::string(TROJAN_PLUGIN_NAME)};
        ProxyPlugin redirPlugin{.name = "r", .plugin = std::string(REDIR_PLUGIN_NAME)};
        ProxyPlugin tlsPlugin{.name = "t", .plugin = std::string(TLS_PLUGIN_NAME)};
        std::string userinfo(uri.userInfo);
        uriUnescapeInPlaceExA(userinfo.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        std::string const password(userinfo.data());
        trojanPlugin.param = nlohmann::json::to_cbor(
            {{"password", nlohmann::json::binary_t(std::vector<uint8_t>(password.begin(), password.end()))},
             {"tls_next", "r.tcp"}});
        uint16_t port{};
        if (uri.portText.empty())
        {
            port = 443;
        }
        else
        {
            auto const parsedPort = ParsePort(uri.portText);
            if (!parsedPort.has_value())
            {
                return std::nullopt;
            }
            port = *parsedPort;
        }
        redirPlugin.param = nlohmann::json::to_cbor(
            {{"dest", {{"host", uri.hostText}, {"port", port}}}, {"tcp_next", "t.tcp"}, {"udp_next", "$out.udp"}});
        nlohmann::json tlsParam{{"next", "$out.tcp"}};
        auto const queryMap = ParseQuery(uri.query);
        if (queryMap.contains("allowInsecure") && queryMap.at("allowInsecure") == "1")
        {
            tlsParam["skip_cert_check"] = true;
        }
        if (queryMap.contains("peer"))
        {
            tlsParam["sni"] = queryMap.at("peer");
        }
        tlsPlugin.param = nlohmann::json::to_cbor(std::move(tlsParam));

        DynOutboundV1Proxy proxy{.tcp_entry = "p.tcp",
                                 .udp_entry = std::nullopt, // TODO: Trojan UDP
                                 .plugins = {std::move(trojanPlugin), std::move(redirPlugin), std::move(tlsPlugin)}};
        std::string fragment(uri.fragment);
        uriUnescapeInPlaceExA(fragment.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        std::string const proxyName(fragment.data());
        return std::make_pair(std::move(proxyName), nlohmann::json::to_cbor(proxy));
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

    ParsedUri ParsedUri::fromUri(UriUriA const &uri)
    {
        ParsedUri parsedUri{.scheme = UriTextRangeAToStringView(uri.scheme),
                            .userInfo = UriTextRangeAToStringView(uri.userInfo),
                            .hostText = UriTextRangeAToStringView(uri.hostText),
                            .portText = UriTextRangeAToStringView(uri.portText),
                            .query = UriTextRangeAToStringView(uri.query),
                            .fragment = UriTextRangeAToStringView(uri.fragment)};

        auto pathPtr = uri.pathHead;
        while (pathPtr != nullptr)
        {
            UriTextRangeA const &pathText = pathPtr->text;
            parsedUri.pathSegments.emplace_back(UriTextRangeAToStringView(pathText));
            pathPtr = pathPtr->next;
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
            if (plugin.plugin == HTTP_OBFS_PLUGIN_NAME || plugin.plugin == TLS_OBFS_PLUGIN_NAME)
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
            if (transportPlugin.has_value() || tlsPlugin.has_value())
            {
                return std::nullopt;
            }
            return ConvertSsToLink(name, protocolPlugin.value(), redirectPlugin.value());
        }
        if (protocolPlugin.value().get().plugin == TROJAN_PLUGIN_NAME)
        {
            if (transportPlugin.has_value() || !tlsPlugin.has_value())
            {
                return std::nullopt;
            }
            return ConvertTrojanToLink(name, protocolPlugin.value(), redirectPlugin.value(), tlsPlugin.value());
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
                                               ProxyPlugin const &redirectPlugin)
    {
        std::string method, host, port;
        nlohmann::json::binary_t password;
        try
        {
            auto const redirParam = nlohmann::json::from_cbor(redirectPlugin.param);
            auto const ssParam = nlohmann::json::from_cbor(protocolPlugin.param);
            method = ssParam.at("method");
            password = ssParam.at("password");
            host = redirParam.at("dest").at("host");
            port = std::to_string(static_cast<int>(redirParam.at("dest").at("port")));
        }
        catch (...)
        {
            return std::nullopt;
        }
        auto const userinfo =
            base64_encode(method + ':' + std::string(reinterpret_cast<char *>(password.data()), password.size()));

        std::vector<char> escapedName((name.size() + 1) * 3);
        uriEscapeExA(name.data(), name.data() + name.size(), escapedName.data(), URI_TRUE, URI_FALSE);

        UriUriA uri{.scheme = StringViewToUriTextRangeA(SS_SCHEME),
                    .userInfo = StringViewToUriTextRangeA(userinfo),
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .fragment = StringViewToUriTextRangeA(std::string_view(escapedName.data()))};
        return ComposeUri(uri);
    }
    std::optional<std::string> ConvertTrojanToLink(std::string_view name, ProxyPlugin const &protocolPlugin,
                                                   ProxyPlugin const &redirectPlugin, ProxyPlugin const &tlsPlugin)
    {
        std::string host, port;
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
            if (tlsParam.contains("skip_cert_check") && tlsParam.at("skip_cert_check") == true)
            {
                allowInsecure = true;
            }
        }
        catch (...)
        {
            return std::nullopt;
        }

        std::string querystring;
        UriQueryListA *queryList = nullptr, peerQuery = {.key = "peer", .value = "", .next = nullptr},
                      allowInsecureQuery = {.key = "allowInsecure", .value = "1", .next = nullptr};
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
        if (queryList != nullptr)
        {
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
            querystring = std::string(qs.data(), qsCharsWritten - 1);
        }

        std::vector<char> escapedName((name.size() + 1) * 3);
        uriEscapeExA(name.data(), name.data() + name.size(), escapedName.data(), URI_TRUE, URI_FALSE);

        UriUriA uri{.scheme = StringViewToUriTextRangeA(TROJAN_SCHEME),
                    .userInfo =
                        UriTextRangeA{.first = reinterpret_cast<char const *>(password.data()),
                                      .afterLast = reinterpret_cast<char const *>(password.data()) + password.size()},
                    .hostText = StringViewToUriTextRangeA(host),
                    .portText = StringViewToUriTextRangeA(port),
                    .query = StringViewToUriTextRangeA(querystring),
                    .fragment = StringViewToUriTextRangeA(std::string_view(escapedName.data()))};
        return ComposeUri(uri);
    }
}
