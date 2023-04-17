#pragma once
#include "ProxyModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProxyModel : ProxyModelT<ProxyModel>
    {
        ProxyModel() = default;
        ProxyModel(FfiProxy const &proxy)
            : m_id(proxy.id), m_name(to_hstring(proxy.name)), m_proxy(proxy.proxy), m_proxyVersion(proxy.proxy_version)
        {
        }

        uint32_t Id() const;
        hstring Name() const;
        void Name(hstring const &value);
        com_array<uint8_t> Proxy() const;
        void Proxy(array_view<uint8_t const> value);
        uint16_t ProxyVersion() const;
        void ProxyVersion(uint16_t value);
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;

      private:
        uint32_t m_id;
        hstring m_name;
        std::vector<uint8_t> m_proxy;
        uint16_t m_proxyVersion;
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
