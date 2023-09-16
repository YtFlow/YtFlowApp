#include "pch.h"
#include "NetifHomeWidget.h"
#if __has_include("NetifHomeWidget.g.cpp")
#include "NetifHomeWidget.g.cpp"
#endif

#include <winsock2.h>
#include <ws2ipdef.h>

#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    NetifHomeWidget::NetifHomeWidget()
    {
        InitializeComponent();
    }

    NetifHomeWidget::NetifHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo)
        : m_sharedInfo(std::move(sharedInfo))
    {
        InitializeComponent();

        PluginNameText().Text(std::move(pluginName));
    }

    void NetifHomeWidget::UpdateInfo()
    {
        try
        {
            NetifInfo info{nlohmann::json::from_cbor(*m_sharedInfo)};

            PreviewInterfaceNameText().Text(to_hstring(info.netif.name));
            InterfaceNameText().Text(to_hstring(info.netif.name));
            if (info.netif.ipv4_addr.has_value())
            {
                sockaddr_in addr;
                memset(&addr, 0, sizeof(sockaddr_in));
                addr.sin_family = AF_INET;
                if (memcpy_s(&addr.sin_addr, sizeof(IN_ADDR), info.netif.ipv4_addr->first.data(), 4))
                {
                    throw hresult_out_of_bounds{};
                }
                addr.sin_port = info.netif.ipv4_addr->second;
                std::array<wchar_t, 64> buf{};
                DWORD bufSize{static_cast<DWORD>(buf.size())};
                if (FAILED(WSAAddressToString((LPSOCKADDR)&addr, sizeof(sockaddr_in), nullptr, buf.data(), &bufSize)))
                {
                    throw_last_error();
                }
                Ipv4AddrText().Text(hstring(buf.data(), bufSize));
            }
            if (info.netif.ipv6_addr.has_value())
            {
                sockaddr_in6 addr;
                memset(&addr, 0, sizeof(sockaddr_in6));
                addr.sin6_family = AF_INET6;
                if (memcpy_s(&addr.sin6_addr, sizeof(IN6_ADDR), info.netif.ipv6_addr->first.data(), 16))
                {
                    throw hresult_out_of_bounds{};
                }
                addr.sin6_port = info.netif.ipv6_addr->second;
                std::array<wchar_t, 128> buf{};
                DWORD bufSize{static_cast<DWORD>(buf.size())};
                if (FAILED(WSAAddressToString((LPSOCKADDR)&addr, sizeof(sockaddr_in6), nullptr, buf.data(), &bufSize)))
                {
                    throw_last_error();
                }
                Ipv6AddrText().Text(hstring(buf.data(), bufSize));
            }

            hstring dnsServers;
            for (auto const &dns : info.netif.dns_servers)
            {
                dnsServers = dnsServers + to_hstring(dns) + L", ";
            }
            if (dnsServers.ends_with(L", "))
            {
                dnsServers =
                    static_cast<std::wstring_view>(dnsServers).substr(0, static_cast<size_t>(dnsServers.size()) - 2);
            }
            DnsText().Text(dnsServers);
        }
        catch (...)
        {
            NotifyException(L"Updating Netif");
        }
    }
}
