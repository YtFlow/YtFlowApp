#pragma once

#include <uriparser/Uri.h>

namespace winrt::YtFlowApp::implementation
{
    constexpr static std::string_view SS_SCHEME = "ss";
    constexpr static char SS_WITH_SCHEME[6] = "ss://";
    constexpr static std::string_view TROJAN_SCHEME = "trojan";
    constexpr static std::string_view SS_PLUGIN_NAME = "shadowsocks-client";
    constexpr static std::string_view TROJAN_PLUGIN_NAME = "trojan-client";
    constexpr static std::string_view TLS_PLUGIN_NAME = "tls-client";
    constexpr static std::string_view HTTP_OBFS_PLUGIN_NAME = "http-obfs-client";
    constexpr static std::string_view TLS_OBFS_PLUGIN_NAME = "tls-obfs-client";
    constexpr static std::string_view REDIR_PLUGIN_NAME = "redirect";
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
        if (auto const u = json.at("udp_entry"); u.is_string())
        {
            r.udp_entry = u.get<std::string>();
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
    struct ParsedUri
    {
        std::string_view scheme, userInfo, hostText, portText, query, fragment;
        std::vector<std::string_view> pathSegments;

        static ParsedUri fromUri(UriUriA const &uri);
    };

    std::optional<uint16_t> ParsePort(std::string_view portText);
    std::map<std::string, std::string> ParseQuery(std::string_view uriQuery);
    std::optional<std::string> ComposeUri(UriUriA const &uri);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertUriToProxy(ParsedUri const &uri);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertSip002ToProxy(ParsedUri const &uri);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertTrojanUriToProxy(ParsedUri const &uri);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertLegacySsToProxy(ParsedUri const &uri);
    std::optional<std::string> ConvertSsToLink(std::string_view name, ProxyPlugin const &ssPlugin,
                                               ProxyPlugin const &redirPlugin);
    std::optional<std::string> ConvertTrojanToLink(std::string_view name, ProxyPlugin const &trojanPlugin,
                                                   ProxyPlugin const &redirPlugin, ProxyPlugin const &tlsPlugin);
    UriTextRangeA StringViewToUriTextRangeA(std::string_view sv);
    std::string_view UriTextRangeAToStringView(UriTextRangeA const &text);
}
