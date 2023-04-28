#include "pch.h"
#include "CoreProxy.h"

#include <optional>
#include <string>

#include <uriparser/Uri.h>

#include "base64.h"

namespace winrt::YtFlowApp::implementation
{
    const static std::string_view SS_SCHEME = "ss";
    struct ProxyPlugin
    {
        std::string name;
        std::string plugin;
        uint16_t plugin_version{0};
        nlohmann::json::binary_t param;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProxyPlugin, name, plugin, plugin_version, param)
    struct DynOutboundV1Proxy
    {
        std::string tcp_entry;
        std::optional<std::string> udp_entry{std::nullopt};
        std::vector<ProxyPlugin> plugins;
    };
    inline void from_json(nlohmann::json const &json, DynOutboundV1Proxy &r)
    {
        json.at("tcp_entry").get_to(r.tcp_entry);
        json.at("plugins").get_to(r.plugins);
        if (!json.contains("udp_entry"))
        {
            return;
        }
        auto const u = json.at("udp_entry");
        if (u.is_string())
        {
            r.udp_entry = static_cast<std::string>(u);
        }
    }
    inline void to_json(nlohmann::json &json, DynOutboundV1Proxy const &r)
    {
        json = nlohmann::json{{"tcp_entry", r.tcp_entry}, {"plugins", r.plugins}};
        if (r.udp_entry.has_value())
        {
            json["udp_entry"] = r.udp_entry.value();
        }
        else
        {
            json["udp_entry"] = nullptr;
        }
    }

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertUriToProxy(UriUriA const &uri)
    {
        if (std::string_view const scheme(uri.scheme.first, uri.scheme.afterLast); scheme != SS_SCHEME)
        {
            return std::nullopt;
        }
        std::string frag(uri.fragment.first, uri.fragment.afterLast);
        uriUnescapeInPlaceExA(frag.data(), URI_TRUE, URI_BR_DONT_TOUCH);
        std::string proxyName(
            frag); // proxyName may contain a \0 in the middle, which truncates the string across FFI boundary.
        if (proxyName.empty())
        {
            proxyName = "New Proxy";
        }
        ProxyPlugin ssPlugin{.name = "p", .plugin = "shadowsocks-client"};
        ProxyPlugin redirPlugin{.name = "r", .plugin = "redirect"};

        std::string_view const b64Userinfo(uri.userInfo.first, uri.userInfo.afterLast);
        std::string userinfo;
        try
        {
            userinfo = base64_decode(b64Userinfo);
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
        std::string_view const portText(uri.portText.first, uri.portText.afterLast);
        int port{};
        if (auto const [_, ec] = std::from_chars(portText.data(), portText.data() + portText.size(), port);
            ec != std::errc())
        {
            return std::nullopt;
        }

        redirPlugin.param = nlohmann::json::to_cbor(
            {{"dest", {{"host", std::string_view(uri.hostText.first, uri.hostText.afterLast)}, {"port", port}}},
             {"tcp_next", "$out.tcp"},
             {"udp_next", "$out.udp"}});

        DynOutboundV1Proxy proxy{
            .tcp_entry = "p.tcp", .udp_entry = "p.udp", .plugins = {std::move(ssPlugin), std::move(redirPlugin)}};
        return std::make_pair(std::move(proxyName), nlohmann::json::to_cbor(proxy));
    }

    UriTextRangeA StringViewToUriTextRangeA(std::string_view sv)
    {
        return UriTextRangeA{.first = sv.data(), .afterLast = sv.data() + sv.size()};
    }

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertShareLinkToProxy(std::string const &link)
    {
        UriUriA uri;
        char const *errPos;
        if (uriParseSingleUriA(&uri, link.c_str(), &errPos) != 0)
        {
            return std::nullopt;
        }

        auto const ret = ConvertUriToProxy(uri);
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
        if (proxyObj.plugins.size() != 2)
        {
            return std::nullopt;
        }
        auto const redirectPluginRes =
            std::ranges::find_if(proxyObj.plugins, [](auto const &p) { return p.plugin == "redirect"; });
        if (redirectPluginRes == proxyObj.plugins.end())
        {
            return std::nullopt;
        }
        auto const protocolPluginRes =
            std::ranges::find_if(proxyObj.plugins, [](auto const &p) { return p.plugin != "redirect"; });
        if (protocolPluginRes == proxyObj.plugins.end())
        {
            return std::nullopt;
        }
        auto const &[redirectPlugin, protocolPlugin] =
            std::make_pair(std::cref(*redirectPluginRes), std::cref(*protocolPluginRes));
        if (protocolPlugin.plugin != "shadowsocks-client")
        {
            return std::nullopt;
        }
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
}
