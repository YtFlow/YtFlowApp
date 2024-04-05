#pragma once
#include "ProxyModel.g.h"

#include "CoreFfi.h"
#include "CoreProxy.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProxyModel : ProxyModelT<ProxyModel>
    {
        ProxyModel() = default;
        ProxyModel(FfiDataProxy const &proxy)
            : m_id(proxy.id), m_name(to_hstring(proxy.name)), m_proxy(proxy.proxy), m_proxyVersion(proxy.proxy_version)
        {
            Analyze(proxy.name.c_str());
        }

        uint32_t Id() const;
        hstring Name() const;
        void Name(hstring const &value);
        com_array<uint8_t> Proxy() const;
        void Proxy(array_view<uint8_t const> value);
        uint16_t ProxyVersion() const;
        void ProxyVersion(uint16_t value);
        hstring Summary() const;
        hstring Tooltip() const;
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;
        std::optional<FfiProxy> &Analyze(char const *name = nullptr) const;

        void Update() const;

      private:
        uint32_t m_id;
        hstring m_name;
        std::vector<uint8_t> m_proxy;
        uint16_t m_proxyVersion;
        mutable std::optional<FfiProxy> m_analyzedProxy;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
