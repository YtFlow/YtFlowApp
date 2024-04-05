#include "pch.h"
#include "ProxyModel.h"
#include "ProxyModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    template <class... Ts> struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;
    using ytflow_core::ytflow_app_proxy_data_proxy_analyze;

    uint32_t ProxyModel::Id() const
    {
        return m_id;
    }
    hstring ProxyModel::Name() const
    {
        return m_name;
    }
    void ProxyModel::Name(hstring const &value)
    {
        m_name = value;
        m_analyzedProxy = std::nullopt;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Name"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Summary"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Tooltip"));
    }
    com_array<uint8_t> ProxyModel::Proxy() const
    {
        return com_array(m_proxy);
    }
    void ProxyModel::Proxy(array_view<uint8_t const> value)
    {
        m_proxy = std::vector(value.begin(), value.end());
        m_analyzedProxy = std::nullopt;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Proxy"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Summary"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Tooltip"));
    }
    uint16_t ProxyModel::ProxyVersion() const
    {
        return m_proxyVersion;
    }
    void ProxyModel::ProxyVersion(uint16_t value)
    {
        m_proxyVersion = value;
        m_analyzedProxy = std::nullopt;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"ProxyVersion"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Summary"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Tooltip"));
    }

    hstring ProxyModel::Summary() const
    {
        auto const &proxy = Analyze();
        if (!proxy.has_value())
        {
            return L"";
        }

        auto const &legs = proxy.value().legs;
        if (legs.empty())
        {
            return L"Direct";
        }

        hstring ret;
        auto pushLeg = [&](FfiProxyLeg const &leg) {
            ret = std::move(ret) +
                  std::visit(overloaded{
                                 [](FfiProxyProtocolShadowsocks const &) { return L"Shadowsocks("; },
                                 [](FfiProxyProtocolTrojan const &) { return L"Trojan("; },
                                 [](FfiProxyProtocolSocks5 const &) { return L"SOCKS5("; },
                                 [](FfiProxyProtocolHttp const &) { return L"HTTP("; },
                                 [](FfiProxyProtocolVMess const &) { return L"VMess("; },
                             },
                             leg.protocol) +
                  to_hstring(leg.dest.host) + L":" + to_hstring(leg.dest.port) + L")";
            if (leg.obfs.has_value())
            {
                ret = std::move(ret) + std::visit(overloaded{
                                                      [](FfiProxyObfsHttpObfs const &) { return L", simple-obfs"; },
                                                      [](FfiProxyObfsTlsObfs const &) { return L", simple-obfs"; },
                                                      [](FfiProxyObfsWebSocket const &) { return L", WebSocket"; },
                                                  },
                                                  leg.obfs.value());
            }
            if (leg.tls.has_value())
            {
                ret = std::move(ret) + L", TLS";
            }
        };
        pushLeg(legs.front());
        for (auto const &leg : std::span(legs.begin() + 1, legs.end()))
        {
            ret = std::move(ret) + L" -> ";
            pushLeg(leg);
        }
        return ret;
    }

    hstring ProxyModel::Tooltip() const
    {
        return Name() + L"\r\n" + Summary();
    }

    event_token ProxyModel::PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProxyModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    std::optional<FfiProxy> &ProxyModel::Analyze(char const *name) const
    {
        if (m_analyzedProxy.has_value())
        {
            return m_analyzedProxy;
        }

        std::optional<std::string> nameTmp;
        if (name == nullptr)
        {
            nameTmp = to_string(m_name);
            name = nameTmp->c_str();
        }
        try
        {
            m_analyzedProxy = std::make_optional(unwrap_ffi_buffer<FfiProxy>(
                ytflow_app_proxy_data_proxy_analyze(name, m_proxy.data(), m_proxy.size(), m_proxyVersion)));
        }
        catch (FfiException const &)
        {
        }

        return m_analyzedProxy;
    }

    void ProxyModel::Update() const
    {
        auto conn{FfiDbInstance.Connect()};
        conn.UpdateProxy(m_id, to_string(m_name).data(), m_proxy.data(), m_proxy.size(), m_proxyVersion);
    }
}
