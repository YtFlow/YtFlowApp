#include "pch.h"
#include "PluginTypeToDescConverter.h"
#include "PluginTypeToDescConverter.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::YtFlowApp::implementation
{
    std::map<hstring, Windows::Foundation::IInspectable> const PluginTypeToDescConverter::DescMap{
        {L"reject", box_value(L"Silently reject any incoming requests.")},
        {L"null", box_value(L"Silently drop any outgoing requests.")},
        {L"ip-stack", box_value(L"Handle TCP or UDP connections from a TUN.")},
        {L"socket-listener", box_value(L"Bind a socket to a specified port and listen for connections or datagrams.")},
        {L"vpn-tun",
         box_value(L"An instance to be instantiated by a VPN system service, such as UWP VPN Plugin on Windows.")},
        {L"host-resolver", box_value(L"Resolve real IP addresses by querying DNS servers.")},
        {L"fake-ip",
         box_value(L"Assign a fake IP address for each domain name. This is useful for TUN inbounds where incoming "
                   L"connections carry no information about domain names. By using a Fake IP resolver, destination "
                   L"IP addresses can be mapped back to a domain name that the client is connecting to.")},
        {L"system-resolver",
         box_value(L"Resolve real IP addresses by calling system functions. This is the recommended resolver for "
                   L"simple proxy scenarios for both client and server.")},
        {L"switch", box_value(L"Handle incoming connections using runtime-selected handlers from a pre-defined list.")},
        {L"dns-server",
         box_value(L"Respond to DNS request messages using results returned by the specified resolver.")},
        {L"socks5-server", box_value(L"SOCKS5 server.")},
        {L"http-obfs-server", box_value(L"simple-obfs HTTP server.")},
        {L"resolve-dest", box_value(L"Resolve domain names in flow destinations from/to IP addresses.")},
        {L"simple-dispatcher", box_value(L"Match the source/dest address against a list of simple rules, and use "
                                         L"the corresponding handler or fallback handler if there is no match.")},
        {L"forward",
         box_value(L"Establish a new connection for each incoming connection, and forward data between them.")},
        {L"dyn-outbound", box_value(L"Select an outbound proxy from the database at runtime.")},
        {L"shadowsocks-client", box_value(L"Shadowsocks client.")},
        {L"socks5-client", box_value(L"SOCKS5 client.")},
        {L"http-proxy-client", box_value(L"HTTP Proxy client. Use HTTP CONNECT to connect to the proxy server.")},
        {L"tls-client", box_value(L"TLS client stream.")},
        {L"trojan-client", box_value(L"Trojan client. Note that TLS is not included. You will likely need to "
                                     L"connect this plugin to a TLS plugin.")},
        {L"http-obfs-client", box_value(L"simple-obfs HTTP client.")},
        {L"redirect", box_value(L"Change the destination of connections or datagrams.")},
        {L"socket", box_value(L"Represents a system socket connection.")},
        {L"netif", box_value(L"A dynamic network interface.")}};

    IInspectable PluginTypeToDescConverter::Convert(IInspectable const &value, TypeName const & /* targetType */,
                                                    IInspectable const & /* parameter */,
                                                    hstring const & /* language */)
    {
        auto const pluginType{unbox_value<hstring>(value)};
        auto const it{DescMap.find(pluginType)};
        if (it == DescMap.end())
        {
            return value;
        }
        return box_value(it->second);
    }
    IInspectable PluginTypeToDescConverter::ConvertBack(IInspectable const & /* value */,
                                                        TypeName const & /* targetType */,
                                                        IInspectable const & /* parameter */,
                                                        hstring const & /* language */)
    {
        throw hresult_not_implemented();
    }
}
