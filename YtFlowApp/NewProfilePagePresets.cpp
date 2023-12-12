#include "pch.h"

#include "NewProfilePage.h"

using namespace nlohmann;

namespace winrt::YtFlowApp::implementation
{
    json::object_t GenIpStack()
    {
        return {{"desc", "Handle TCP or UDP connections from UWP VPN."},
                {"plugin", "ip-stack"},
                {"plugin_version", 0},
                {"param",
                 {{"tcp_next", "fakeip-dns-server.tcp_map_back.udp-proxy-switch.tcp"},
                  {"udp_next", "dns-dispatcher.udp"},
                  {"tun", "uwp-vpn-tun.tun"}}}};
    }
    json::object_t GenUwpVpnTun()
    {
        return {{"desc", "UWP VPN Plugin TUN interface."},
                {"plugin", "vpn-tun"},
                {"plugin_version", 0},
                {"param",
                 {{"dns", nlohmann::json::array({"11.16.1.1", "260c:1::1"})},
                  {"ipv4", "192.168.3.1"},
                  {"ipv4_route",
                   nlohmann::json::array({"11.17.0.0/16", "11.16.0.0/16", "149.154.160.0/20", "91.108.56.130/32"})},
                  {"ipv6", "fd00::2"},
                  {"ipv6_route", nlohmann::json::array({"::/16", "260c:2001::/96", "260c:1::/96", "2001:0b28:f23c::/46",
                                                        "2001:067c:04e8::/48"})},
                  {"web_proxy", nullptr}}}};
    }
    json::object_t GenForward(char const *desc, char const *tcpNext, char const *udpNext = "phy.udp")
    {
        return {{"desc", desc},
                {"plugin", "forward"},
                {"plugin_version", 0},
                {"param", {{"request_timeout", 200}, {"tcp_next", tcpNext}, {"udp_next", udpNext}}}};
    }
    json::object_t GenDnsDispatcher()
    {
        return {
            {"desc", "Dispatches DNS requests to our DNS server."},
            {"plugin", "simple-dispatcher"},
            {"plugin_version", 0},
            {"param",
             {{"fallback_tcp", "reject.tcp"},
              {"fallback_udp", "fakeip-dns-server.udp_map_back.udp-proxy-switch.udp"},
              {"rules",
               {{{"src", {{"ip_ranges", {"0.0.0.0/0", "::/0"}}, {"port_ranges", {{{"start", 0}, {"end", 65535}}}}}},
                 {"dst",
                  {{"ip_ranges", {"11.16.1.1/32", "260c:1::1/128"}}, {"port_ranges", {{{"start", 53}, {"end", 53}}}}}},
                 {"is_udp", true},
                 {"next", "fakeip-dns-server.udp"}}}}}}};
    }
    json NewProfilePage::GenPresetDoc()
    {
        return {
            {"entry-ip-stack", GenIpStack()},
            {"uwp-vpn-tun", GenUwpVpnTun()},
            {"proxy-forward", GenForward("Forward connections to the proxy outbound.", "outbound.tcp", "outbound.udp")},
            {"direct-forward", GenForward("Forward connections to the physical outbound.", "phy.tcp")},
            {
                "outbound",
                {{"desc", "Allows runtime selection of outbound proxies from the Library."},
                 {"plugin", "dyn-outbound"},
                 {"plugin_version", 0},
                 {"param", {{"tcp_next", "phy.tcp"}, {"udp_next", "phy.udp"}}}},
            },
            {
                "ss-client",
                {{"desc", "Shadowsocks client."},
                 {"plugin", "shadowsocks-client"},
                 {"plugin_version", 0},
                 {"param",
                  {{"method", "aes-128-gcm"},
                   {"password", {{"__byte_repr", "utf8"}, {"data", "my_ss_password"}}},
                   {"tcp_next", "proxy-redir.tcp"},
                   {"udp_next", "null.udp"}}}},
            },
            {
                "http-proxy-client",
                {{"desc", "HTTP Proxy client. Use HTTP CONNECT to connect to the proxy server."},
                 {"plugin", "http-proxy-client"},
                 {"plugin_version", 0},
                 {"param",
                  {{"tcp_next", "proxy-redir.tcp"},
                   {"user", {{"__byte_repr", "utf8"}, {"data", ""}}},
                   {"pass", {{"__byte_repr", "utf8"}, {"data", ""}}}}}},
            },
            {
                "trojan-client",
                {{"desc", "Trojan client. The TLS part is in plugin client-tls."},
                 {"plugin", "trojan-client"},
                 {"plugin_version", 0},
                 {"param",
                  {{"password", {{"__byte_repr", "utf8"}, {"data", "my_trojan_password"}}},
                   {"tls_next", "proxy-redir.tcp"}}}},
            },
            {
                "client-tls",
                {{"desc", "TLS client stream for proxy client."},
                 {"plugin", "tls-client"},
                 {"plugin_version", 0},
                 {"param", {{"next", "phy.tcp"}, {"skip_cert_check", false}, {"sni", "my.proxy.server.com"}}}},
            },
            {
                "vmess-client",
                {{"desc", "VMess client."},
                 {"plugin", "vmess-client"},
                 {"plugin_version", 0},
                 {"param",
                  {{"user_id", "b831381d-6324-4d53-ad4f-8cda48b30811"},
                   {"security", "auto"},
                   {"alter_id", 0},
                   {"tcp_next", "proxy-redir.tcp"}}}},
            },
            {
                "ws-client",
                {{"desc", "WebSocket client."},
                 {"plugin", "ws-client"},
                 {"plugin_version", 0},
                 {"param",
                  {{"host", "dl.microsoft.com"},
                   {"path", "/"},
                   {"headers", json::object()},
                   {"next", "client-tls.tcp"}}}},
            },
            {
                "proxy-redir",
                {{"desc", "Change the destination to the proxy server."},
                 {"plugin", "redirect"},
                 {"plugin_version", 0},
                 {"param",
                  {{"dest", {{"host", "my.proxy.server.com."}, {"port", 8388}}},
                   {"tcp_next", "phy.tcp"},
                   {"udp_next", "phy.udp"}}}},
            },
            {
                "phy",
                {{"desc", "The physical NIC."},
                 {"plugin", "netif"},
                 {"plugin_version", 0},
                 {"param", {{"family_preference", "Both"}, {"type", "Auto"}}}},
            },
            {
                "null",
                {
                    {"desc", "Return an error for any incoming requests."},
                    {"plugin", "null"},
                    {"plugin_version", 0},
                    {"param", nullptr} // Represents a null value
                },
            },
            {
                "reject",
                {
                    {"desc", "Silently drop any outgoing requests."},
                    {"plugin", "reject"},
                    {"plugin_version", 0},
                    {"param", nullptr} // Represents a null value
                },
            },
            {
                "fake-ip",
                {{"desc", "Assign a fake IP address for each domain name. This is useful for TUN inbounds where "
                          "incoming "
                          "connections carry no information about domain names. By using a Fake IP resolver, "
                          "destination "
                          "IP addresses can be mapped back to a domain name that the client is connecting to."},
                 {"plugin", "fake-ip"},
                 {"plugin_version", 0},
                 {"param",
                  {{"fallback", "null.resolver"},
                   {"prefix_v4", {11, 17}},
                   {"prefix_v6", {38, 12, 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}}}},
            },
            {
                "doh-resolver",
                {{"desc", "Resolves real IP addresses from a secure, trusted provider."},
                 {"plugin", "host-resolver"},
                 {"plugin_version", 0},
                 {"param",
                  {{"udp", nlohmann::json::array()},
                   {"tcp", nlohmann::json::array()},
                   {"doh", {{{"url", "https://1.1.1.1/dns-query"}, {"next", "general-tls.tcp"}}}}}}},
            },
            {"dns-dispatcher", GenDnsDispatcher()},
            {"rule-switch",
             {{"desc", "Decide whether rules should take effect at runtime."},
              {"plugin", "switch"},
              {"plugin_version", 0},
              {"param",
               {{
                   "choices",
                   {
                       {{"name", "Rule"},
                        {"description", "Match connections against rules"},
                        {"tcp_next", "custom-rule-dispatcher.tcp"},
                        {"udp_next", "custom-rule-dispatcher.udp"}},
                       {{"name", "Proxy"},
                        {"description", "Proxy all connections unconditionally"},
                        {"tcp_next", "proxy-forward.tcp"},
                        {"udp_next", "resolve-proxy.udp"}},
                       {{"name", "Direct"},
                        {"description", "Connections will not go through any proxies"},
                        {"tcp_next", "direct-forward.tcp"},
                        {"udp_next", "resolve-local.udp"}},
                   },
               }}}}},
            {"udp-proxy-switch",
             {{"desc", "Decide whether UDP packets should go through proxies."},
              {"plugin", "switch"},
              {"plugin_version", 0},
              {"param",
               {{
                   "choices",
                   {
                       {{"name", "On"},
                        {"description", "UDP packets will go through the same routing decisions as TCP "
                                        "connections, possibly via a proxy"},
                        {"tcp_next", "rule-switch.tcp"},
                        {"udp_next", "rule-switch.udp"}},
                       {{"name", "Off"},
                        {"description", "UDP packets will not go through any rules or proxies"},
                        {"tcp_next", "rule-switch.tcp"},
                        {"udp_next", "resolve-local.udp"}},
                   },
               }}}}},
            {
                "general-tls",
                {{"desc", "TLS client stream for h2, DoH etc."},
                 {"plugin", "tls-client"},
                 {"plugin_version", 0},
                 {"param", {{"next", "phy.tcp"}, {"skip_cert_check", false}}}},
            },
            {
                "fakeip-dns-server",
                {{"desc", "Respond to DNS request messages using results from the FakeIP resolver."},
                 {"plugin", "dns-server"},
                 {"plugin_version", 0},
                 {"param",
                  {{"concurrency_limit", 64},
                   {"resolver", "fakeip-filter.resolver"},
                   {"tcp_map_back", {"udp-proxy-switch.tcp"}},
                   {"udp_map_back", {"udp-proxy-switch.udp"}},
                   {"ttl", 60}}}},
            },
            {"resolve-local",
             {{"desc", "Resolve domain names to IP addresses for direct connections and establish a mapping to "
                       "preserve the original destination address."},
              {"plugin", "resolve-dest"},
              {"plugin_version", 0},
              {"param",
               {{"resolver", "phy.resolver"},
                {"tcp_next", "direct-forward.tcp"},
                {"udp_next", "direct-forward.udp"}}}}},
            {"resolve-proxy",
             {{"desc", "Resolve domain names to IP addresses for proxied connections through proxy outbound and "
                       "establish a mapping to preserve the original destination address."},
              {"plugin", "resolve-dest"},
              {"plugin_version", 0},
              {"param",
               {{"resolver", "proxy-resolver.resolver"},
                {"tcp_next", "proxy-forward.tcp"},
                {"udp_next", "proxy-forward.udp"}}}}},
            {"proxy-resolver",
             {{"desc", "Resolve real IP addresses via proxy outbound."},
              {"plugin", "host-resolver"},
              {"plugin_version", 0},
              {"param", {{"tcp", json::array_t()}, {"udp", {"proxy-redir-8888.udp"}}}}}},
            {"proxy-redir-8888",
             {{"desc", "Rewrite destination address to 8.8.8.8."},
              {"plugin", "redirect"},
              {"plugin_version", 0},
              {"param",
               {{"dest", {{"host", "8.8.8.8"}, {"port", 53}}},
                {"tcp_next", "outbound.tcp"},
                {"udp_next", "outbound.udp"}}}}}};
    }
}
