#pragma once

#include <uriparser/Uri.h>

namespace winrt::YtFlowApp::implementation
{
    constexpr static std::string_view SS_SCHEME = "ss";
    constexpr static char SS_WITH_SCHEME[6] = "ss://";
    constexpr static std::string_view TROJAN_SCHEME = "trojan";
    constexpr static std::string_view SOCKS5_SCHEME = "socks5";
    constexpr static std::string_view SOCKS5H_SCHEME = "socks5h";
    constexpr static std::string_view HTTP_SCHEME = "http";
    constexpr static std::string_view HTTPS_SCHEME = "https";
    constexpr static std::string_view VMESS_SCHEME = "vmess";
    constexpr static std::string_view SS_PLUGIN_NAME = "shadowsocks-client";
    constexpr static std::string_view TROJAN_PLUGIN_NAME = "trojan-client";
    constexpr static std::string_view SOCKS5_PLUGIN_NAME = "socks5-client";
    constexpr static std::string_view HTTP_PROXY_PLUGIN_NAME = "http-proxy-client";
    constexpr static std::string_view VMESS_PLUGIN_NAME = "vmess-client";
    constexpr static std::string_view TLS_PLUGIN_NAME = "tls-client";
    constexpr static std::string_view HTTP_OBFS_PLUGIN_NAME = "http-obfs-client";
    constexpr static std::string_view TLS_OBFS_PLUGIN_NAME = "tls-obfs-client";
    constexpr static std::string_view WS_PLUGIN_NAME = "ws-client";
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
        static ParsedUri fromUri(UriUriA const &uri);

        std::optional<std::reference_wrapper<std::string>> GetQueryValue(std::string const &key) const;
        bool HasUnvisitedQuery() const noexcept;

        std::string_view scheme, userInfo, hostText, portText, query, fragment;
        std::vector<std::string_view> pathSegments{};

      private:
        std::map<std::string, std::pair<std::string, bool>> mutable queryMap{};
    };

    enum class [[nodiscard]] PluginDecodeResult
    {
        Success,
        Ignore,
        Fail,
    };

    struct Sip002Decoder
    {
        static PluginDecodeResult DecodeSip003Plugin(std::string_view pluginName, std::string_view pluginOpts,
                                                     std::string_view defaultHost, ProxyPlugin &plugin,
                                                     std::string_view tcpNext);

        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext) const;
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext) const;
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext);
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);
    };
    struct LegacyShadowsocksDecoder
    {
        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext);
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext);
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext);
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);

      private:
        [[nodiscard]] bool decodeUserinfoParts(ParsedUri const &uri);

        bool m_userinfoPartsDecoded = false;
        std::tuple<std::string, std::vector<uint8_t>, std::string, std::string> m_userinfoParts{};
    };
    struct TrojanDecoder
    {
        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext) const;
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext); // TODO: obfs
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext) const;
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);
    };
    struct Socks5Decoder
    {
        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext) const;
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext); // TODO: obfs
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext);
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);
    };
    struct HttpDecoder
    {
        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext) const;
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext); // TODO: obfs
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext);
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);
    };
    struct V2raynDecoder
    {
        std::string DecodeName(ParsedUri const &uri);
        PluginDecodeResult DecodeProtocol(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                          std::string_view udpNext);
        PluginDecodeResult DecodeRedir(ParsedUri const &uri, std::string &host, uint16_t &port);
        PluginDecodeResult DecodeObfs(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                      std::string_view udpNext); // TODO: obfs
        PluginDecodeResult DecodeTls(ParsedUri const &uri, ProxyPlugin &plugin, std::string_view tcpNext,
                                     std::string_view udpNext);
        PluginDecodeResult DecodeUdp(ParsedUri const &uri);

      private:
        [[nodiscard]] bool decodeJsonDoc(ParsedUri const &uri);

        bool m_jsonDecoded = false;
        nlohmann::json m_linkDoc{};
    };

    std::optional<uint16_t> ParsePort(std::string_view portText);
    std::map<std::string, std::string> ParseQuery(std::string_view uriQuery);
    std::optional<std::string> ComposeUri(UriUriA const &uri);
    std::string UriEscape(std::string_view uri);
    std::string UriUnescape(std::string uri);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertUriToProxy(ParsedUri const &uri);
    std::optional<std::string> ConvertSsToLink(std::string_view name, ProxyPlugin const &ssPlugin,
                                               ProxyPlugin const &redirPlugin,
                                               std::optional<std::reference_wrapper<ProxyPlugin const>> obfsPlugin);
    std::optional<std::string> ConvertTrojanToLink(std::string_view name, ProxyPlugin const &trojanPlugin,
                                                   ProxyPlugin const &redirPlugin, ProxyPlugin const &tlsPlugin);
    std::optional<std::string> ConvertSocks5ToLink(std::string_view name, ProxyPlugin const &socks5Plugin,
                                                   ProxyPlugin const &redirPlugin);
    std::optional<std::string> ConvertHttpProxyToLink(std::string_view name, ProxyPlugin const &httpProxyPlugin,
                                                      ProxyPlugin const &redirPlugin);
    std::optional<std::string> ConvertVMessProxyToLink(
        std::string_view name, ProxyPlugin const &vmessPlugin, ProxyPlugin const &redirPlugin,
        std::optional<std::reference_wrapper<ProxyPlugin const>> obfsPlugin,
        std::optional<std::reference_wrapper<ProxyPlugin const>> tlsPlugin);
    UriTextRangeA StringViewToUriTextRangeA(std::string_view sv);
    std::string_view UriTextRangeAToStringView(UriTextRangeA const &text);
    std::optional<std::string> ComposeQuery(UriQueryListA const *queryList);

    template <typename D>
    std::optional<std::pair<std::string, std::vector<uint8_t>>> DecodeUriToProxy(ParsedUri const &uri, D &&decoder)
    {
        auto name = decoder.DecodeName(uri);
        std::string_view tcpOut = "$out.tcp", udpOut = "$out.udp";

        DynOutboundV1Proxy proxy{.tcp_entry = "p.tcp", .plugins = {}};

        do
        { // TLS
            ProxyPlugin tlsPlugin{.name = "t", .plugin = std::string(TLS_PLUGIN_NAME)};
            auto const tlsDecodeResult = decoder.DecodeTls(uri, tlsPlugin, tcpOut, udpOut);
            if (tlsDecodeResult == PluginDecodeResult::Ignore)
            {
                break;
            }
            if (tlsDecodeResult == PluginDecodeResult::Fail)
            {
                return std::nullopt;
            }
            proxy.plugins.emplace_back(std::move(tlsPlugin));
            tcpOut = "t.tcp";
        } while (false);
        do
        { // obfs
            ProxyPlugin obfsPlugin{.name = "o"};
            auto const obfsDecodeResult = decoder.DecodeObfs(uri, obfsPlugin, tcpOut, udpOut);
            if (obfsDecodeResult == PluginDecodeResult::Ignore)
            {
                break;
            }
            if (obfsDecodeResult == PluginDecodeResult::Fail)
            {
                return std::nullopt;
            }
            proxy.plugins.emplace_back(std::move(obfsPlugin));
            tcpOut = "o.tcp";
        } while (false);
        do
        { // redir
            std::string host;
            uint16_t port{};
            auto const redirDecodeResult = decoder.DecodeRedir(uri, host, port);
            if (redirDecodeResult == PluginDecodeResult::Ignore)
            {
                break;
            }
            if (redirDecodeResult == PluginDecodeResult::Fail)
            {
                return std::nullopt;
            }
            ProxyPlugin redirPlugin{
                .name = "r",
                .plugin = std::string(REDIR_PLUGIN_NAME),
                .param = nlohmann::json::to_cbor(
                    {{"dest", {{"host", host}, {"port", port}}}, {"tcp_next", tcpOut}, {"udp_next", udpOut}}),
            };
            proxy.plugins.emplace_back(std::move(redirPlugin));
            tcpOut = "r.tcp";
            udpOut = "r.udp";
        } while (false);
        do
        { // protocol
            ProxyPlugin protocolPlugin{.name = "p"};
            auto const protocolDecodeResult = decoder.DecodeProtocol(uri, protocolPlugin, tcpOut, udpOut);
            if (protocolDecodeResult == PluginDecodeResult::Ignore)
            {
                break;
            }
            if (protocolDecodeResult == PluginDecodeResult::Fail)
            {
                return std::nullopt;
            }
            proxy.plugins.emplace_back(std::move(protocolPlugin));
            tcpOut = "p.tcp";
            udpOut = "p.udp";
        } while (false);
        auto const udpDecodeResult = decoder.DecodeUdp(uri);
        if (udpDecodeResult == PluginDecodeResult::Fail)
        {
            return std::nullopt;
        }
        if (udpDecodeResult == PluginDecodeResult::Success)
        {
            proxy.udp_entry = udpOut;
        }

        if (uri.HasUnvisitedQuery())
        {
            return std::nullopt;
        }

        return std::make_pair(std::move(name), nlohmann::json::to_cbor(proxy));
    }
}
