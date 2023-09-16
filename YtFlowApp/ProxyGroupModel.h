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

        uint32_t Id() const;
        hstring Name() const;
        void Name(hstring const &value);
        hstring Type() const;
        void Type(hstring const &value);
        bool IsManualGroup() const;
        bool IsSubscription() const;
        hstring DisplayType() const;
        hstring DisplayTypeIcon() const;
        hstring TooltipText() const;
        hstring SubscriptionUrl() const;
        hstring SubscriptionUploadUsed() const;
        hstring SubscriptionDownloadUsed() const;
        hstring SubscriptionTotalUsed() const;
        double SubscriptionPercentUsed() const;
        bool SubscriptionHasDataUsage() const;
        hstring SubscriptionBytesTotal() const;
        hstring SubscriptionRetrievedAt() const;
        hstring SubscriptionExpireAt() const;
        IObservableVector<ProxyModel> Proxies() const;
        void Proxies(IObservableVector<ProxyModel> const &value);
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

        void AttachSubscriptionInfo(FfiProxyGroupSubscription const &subscription);

        bool IsUpdating{false};

      private:
        uint32_t m_id{};
        hstring m_name;
        hstring m_type;
        hstring m_subscriptionUrl, m_subscriptionUploadUsed, m_subscriptionDownloadUsed, m_subscriptionTotalUsed,
            m_subscriptionBytesTotal, m_subscriptionRetrievedAt, m_subscriptionExpireAt;
        double m_subscriptionPercentUsed{};
        bool m_subscriptionHasDataUsage{false};
        IObservableVector<ProxyModel> m_proxies{nullptr};
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
