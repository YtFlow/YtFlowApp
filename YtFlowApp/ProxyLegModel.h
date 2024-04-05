#pragma once
#include "ProxyLegModel.g.h"

#include "CoreProxy.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProxyLegModel : ProxyLegModelT<ProxyLegModel>
    {
        ProxyLegModel() = default;
        ProxyLegModel(bool isReadonly, FfiProxyLeg const &leg);

        bool IsReadonly() const noexcept;
        bool IsWritable() const noexcept;
        hstring ProtocolType();
        void ProtocolType(hstring const &value);
        hstring ShadowsocksEncryptionMethod();
        void ShadowsocksEncryptionMethod(hstring const &value);
        hstring VMessEncryptionMethod();
        void VMessEncryptionMethod(hstring const &value);
        uint16_t AlterId() const;
        void AlterId(uint16_t value);
        hstring Username();
        void Username(hstring const &value);
        hstring Password();
        void Password(hstring const &value);
        hstring Host();
        void Host(hstring const &value);
        uint16_t Port() const;
        void Port(uint16_t value);
        hstring ObfsType();
        void ObfsType(hstring const &value);
        hstring ObfsHost();
        void ObfsHost(hstring const &value);
        hstring ObfsPath();
        void ObfsPath(hstring const &value);
        hstring ObfsHeaders();
        void ObfsHeaders(hstring const &value);
        bool EnableTls() const;
        void EnableTls(bool value);
        hstring Sni();
        void Sni(hstring const &value);
        hstring Alpn();
        void Alpn(hstring const &value);
        winrt::Windows::Foundation::IReference<bool> SkipCertCheck();
        void SkipCertCheck(winrt::Windows::Foundation::IReference<bool> const &value);
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;
        hstring Dest();
        hstring Summary() const;
        FfiProxyLeg Encode() const;

      private:
        bool m_isReadonly{};
        hstring m_protocolType = L"SOCKS5";
        hstring m_shadowsocksEncryptionMethod = L"none";
        hstring m_vmessEncryptionMethod = L"none";
        uint16_t m_alterId{};
        hstring m_username;
        hstring m_password;
        hstring m_host = L"localhost";
        uint16_t m_port = 1;
        hstring m_obfsType = L"none";
        hstring m_obfsHost;
        hstring m_obfsPath = L"/";
        hstring m_obfsHeaders;
        bool m_enableTls{};
        hstring m_sni = L"auto";
        hstring m_alpn;
        winrt::Windows::Foundation::IReference<bool> m_skipCertCheck;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct ProxyLegModel : ProxyLegModelT<ProxyLegModel, implementation::ProxyLegModel>
    {
    };
}
