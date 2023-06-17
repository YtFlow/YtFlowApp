#pragma once
#include "ProxyGroupModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    using Windows::Foundation::Collections::IObservableVector;
    struct ProxyGroupModel : ProxyGroupModelT<ProxyGroupModel>
    {
        ProxyGroupModel() = default;
        ProxyGroupModel(FfiProxyGroup const &proxyGroup)
            : m_id(proxyGroup.id), m_name(to_hstring(proxyGroup.name)), m_type(to_hstring(proxyGroup.type))
        {
        }
        ProxyGroupModel(FfiProxyGroup const &proxyGroup, FfiProxyGroupSubscription const &subscription)
            : m_id(proxyGroup.id), m_name(to_hstring(proxyGroup.name)), m_type(to_hstring(proxyGroup.type))
        {
        }

        uint32_t Id() const;
        hstring Name() const;
        void Name(hstring const &value);
        hstring Type() const;
        void Type(hstring const &value);
        bool IsManualGroup() const;
        hstring DisplayType() const;
        hstring DisplayTypeIcon() const;
        hstring TooltipText() const;
        IObservableVector<ProxyModel> Proxies() const;
        void Proxies(IObservableVector<ProxyModel> const &value);
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;

      private:
        uint32_t m_id;
        hstring m_name;
        hstring m_type;
        IObservableVector<ProxyModel> m_proxies{nullptr};
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
