#include "pch.h"
#include "ProxyLegModel.h"
#include "ProxyLegModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    template <class... Ts> struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    void ConvertBinaryToHString(std::span<uint8_t const> bytes, hstring &target)
    {
        std::string_view const sv(reinterpret_cast<char const *>(bytes.data()), bytes.size());
        auto hs = to_hstring(sv);
        if (hs.empty() && !sv.empty())
        {
            // TODO: handle raw bytes
            target = L"INVALID_UTF8";
        }
        else
        {
            target = std::move(hs);
        }
    }

    ProxyLegModel::ProxyLegModel(bool isReadonly, FfiProxyLeg const &leg)
    {
        m_isReadonly = isReadonly;
        std::visit(overloaded{
                       [&](FfiProxyProtocolShadowsocks const &v) {
                           m_protocolType = L"Shadowsocks";
                           m_shadowsocksEncryptionMethod = to_hstring(v.cipher);
                           ConvertBinaryToHString(v.password, m_password);
                       },
                       [&](FfiProxyProtocolTrojan const &v) {
                           m_protocolType = L"Trojan";
                           ConvertBinaryToHString(v.password, m_password);
                       },
                       [&](FfiProxyProtocolHttp const &v) {
                           m_protocolType = L"HTTP";
                           ConvertBinaryToHString(v.username, m_username);
                           ConvertBinaryToHString(v.password, m_password);
                       },
                       [&](FfiProxyProtocolSocks5 const &v) {
                           m_protocolType = L"SOCKS5";
                           ConvertBinaryToHString(v.username, m_username);
                           ConvertBinaryToHString(v.password, m_password);
                       },
                       [&](FfiProxyProtocolVMess const &v) {
                           m_protocolType = L"VMess";
                           m_vmessEncryptionMethod = to_hstring(v.security);
                           m_alterId = v.alter_id;

                           GUID userGuid{};
                           assert(v.user_id.size() == sizeof(GUID));
                           std::memmove(&userGuid, v.user_id.data(), sizeof(GUID));
                           std::array<OLECHAR, 37> userIdBuf{};
                           assert(StringFromGUID2(userGuid, userIdBuf.data(), static_cast<int>(userIdBuf.size())) > 0);
                           m_password = hstring(userIdBuf.data());
                       },
                   },
                   leg.protocol);
        m_host = to_hstring(leg.dest.host);
        m_port = leg.dest.port;
        if (leg.obfs.has_value())
        {
            std::visit(overloaded{
                           [&](FfiProxyObfsHttpObfs const &v) {
                               m_obfsType = L"simple-obfs (HTTP)";
                               m_obfsHost = to_hstring(v.host);
                               m_obfsPath = to_hstring(v.path);
                           },
                           [&](FfiProxyObfsTlsObfs const &v) {
                               m_obfsType = L"simple-obfs (TLS)";
                               m_obfsHost = to_hstring(v.host);
                           },
                           [&](FfiProxyObfsWebSocket const &v) {
                               m_obfsType = L"WebSocket";
                               m_obfsHost = to_hstring(v.host.value_or(""));
                               m_obfsPath = to_hstring(v.path);
                               using std::ranges::views::transform;
                               using std::ranges::views::join;
                               using namespace std::string_view_literals;
                               auto headerComponents =
                                   join(v.headers | transform([](auto const &kv) {
                                            return std::array{static_cast<std::string_view>(kv.first), ": "sv,
                                                              static_cast<std::string_view>(kv.second), "\r\n"sv};
                                        }));
                               for (auto const &headerComponent : headerComponents)
                               {
                                   m_obfsHeaders = std::move(m_obfsHeaders) + to_hstring(headerComponent);
                               }
                           },
                       },
                       leg.obfs.value());
        }
        else
        {
            m_obfsType = L"none";
        }
        m_enableTls = leg.tls.has_value();
        if (m_enableTls)
        {
            if (leg.tls->sni.has_value())
            {
                m_sni = to_hstring(leg.tls->sni.value());
            }
            else
            {
                m_sni = L"auto";
            }
            m_skipCertCheck = Windows::Foundation::IReference<bool>(leg.tls->skip_cert_check);
            auto alpnIt = leg.tls->alpn.begin(), alpnEnd = leg.tls->alpn.end();
            if (alpnIt != alpnEnd)
            {
                m_alpn = to_hstring(*alpnIt);
                alpnIt++;
            }
            for (; alpnIt != alpnEnd; alpnIt++)
            {
                m_alpn = std::move(m_alpn) + L",";
                m_alpn = std::move(m_alpn) + to_hstring(*alpnIt);
            }
        }
    }
    bool ProxyLegModel::IsReadonly() const noexcept
    {
        return m_isReadonly;
    }
    bool ProxyLegModel::IsWritable() const noexcept
    {
        return !m_isReadonly;
    }
    hstring ProxyLegModel::ProtocolType()
    {
        return m_protocolType;
    }
    void ProxyLegModel::ProtocolType(hstring const &value)
    {
        if (m_protocolType == value)
        {
            return;
        }
        m_protocolType = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ProtocolType"});
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Summary"});
    }
    hstring ProxyLegModel::ShadowsocksEncryptionMethod()
    {
        return m_shadowsocksEncryptionMethod;
    }
    void ProxyLegModel::ShadowsocksEncryptionMethod(hstring const &value)
    {
        if (m_shadowsocksEncryptionMethod != value)
        {
            m_shadowsocksEncryptionMethod = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ShadowsocksEncryptionMethod"});
        }
    }
    hstring ProxyLegModel::VMessEncryptionMethod()
    {
        return m_vmessEncryptionMethod;
    }
    void ProxyLegModel::VMessEncryptionMethod(hstring const &value)
    {
        if (m_vmessEncryptionMethod != value)
        {
            m_vmessEncryptionMethod = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"VMessEncryptionMethod"});
        }
    }
    uint16_t ProxyLegModel::AlterId() const
    {
        return m_alterId;
    }
    void ProxyLegModel::AlterId(uint16_t value)
    {
        if (m_alterId != value)
        {
            m_alterId = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"AlterId"});
        }
    }
    hstring ProxyLegModel::Username()
    {
        return m_username;
    }
    void ProxyLegModel::Username(hstring const &value)
    {
        if (m_username != value)
        {
            m_username = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Username"});
        }
    }
    hstring ProxyLegModel::Password()
    {
        return m_password;
    }
    void ProxyLegModel::Password(hstring const &value)
    {
        if (m_password != value)
        {
            m_password = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Password"});
        }
    }
    hstring ProxyLegModel::Host()
    {
        return m_host;
    }
    void ProxyLegModel::Host(hstring const &value)
    {
        if (m_host != value)
        {
            m_host = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Host"});
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Dest"});
        }
    }
    uint16_t ProxyLegModel::Port() const
    {
        return m_port;
    }
    void ProxyLegModel::Port(uint16_t value)
    {
        if (m_port != value)
        {
            m_port = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Port"});
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Dest"});
        }
    }
    hstring ProxyLegModel::ObfsType()
    {
        return m_obfsType;
    }
    void ProxyLegModel::ObfsType(hstring const &value)
    {
        if (m_obfsType != value)
        {
            m_obfsType = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ObfsType"});
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Summary"});
        }
    }
    hstring ProxyLegModel::ObfsHost()
    {
        return m_obfsHost;
    }
    void ProxyLegModel::ObfsHost(hstring const &value)
    {
        if (m_obfsHost != value)
        {
            m_obfsHost = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ObfsHost"});
        }
    }
    hstring ProxyLegModel::ObfsPath()
    {
        return m_obfsPath;
    }
    void ProxyLegModel::ObfsPath(hstring const &value)
    {
        if (m_obfsPath != value)
        {
            m_obfsPath = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ObfsPath"});
        }
    }
    hstring ProxyLegModel::ObfsHeaders()
    {
        return m_obfsHeaders;
    }
    void ProxyLegModel::ObfsHeaders(hstring const &value)
    {
        if (m_obfsHeaders != value)
        {
            m_obfsHeaders = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"ObfsHeaders"});
        }
    }
    bool ProxyLegModel::EnableTls() const
    {
        return m_enableTls;
    }
    void ProxyLegModel::EnableTls(bool value)
    {
        if (m_enableTls != value)
        {
            m_enableTls = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"EnableTls"});
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Summary"});
        }
    }
    hstring ProxyLegModel::Sni()
    {
        return m_sni;
    }
    void ProxyLegModel::Sni(hstring const &value)
    {
        if (m_sni != value)
        {
            m_sni = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Sni"});
        }
    }
    hstring ProxyLegModel::Alpn()
    {
        return m_alpn;
    }
    void ProxyLegModel::Alpn(hstring const &value)
    {
        if (m_alpn != value)
        {
            m_alpn = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Alpn"});
        }
    }
    winrt::Windows::Foundation::IReference<bool> ProxyLegModel::SkipCertCheck()
    {
        return m_skipCertCheck;
    }
    void ProxyLegModel::SkipCertCheck(winrt::Windows::Foundation::IReference<bool> const &value)
    {
        if (m_skipCertCheck != value)
        {
            m_skipCertCheck = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"SkipCertCheck"});
        }
    }
    winrt::event_token ProxyLegModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProxyLegModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    hstring ProxyLegModel::Dest()
    {
        return m_host + L":" + to_hstring(m_port);
    }
    hstring ProxyLegModel::Summary() const
    {
        hstring ret = m_protocolType;
        if (!m_obfsType.empty() && m_obfsType != L"none")
        {
            ret = std::move(ret) + L", " + m_obfsType;
        }
        if (m_enableTls)
        {
            ret = std::move(ret) + L", TLS";
        }
        return ret;
    }
}
