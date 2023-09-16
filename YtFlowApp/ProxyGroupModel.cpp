#include "pch.h"
#include "ProxyGroupModel.h"
#include "ProxyGroupModel.g.cpp"

#include "UI.h"

namespace winrt::YtFlowApp::implementation
{
    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;

    uint32_t ProxyGroupModel::Id() const
    {
        return m_id;
    }
    hstring ProxyGroupModel::Name() const
    {
        return m_name;
    }
    void ProxyGroupModel::Name(hstring const &value)
    {
        m_name = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Name"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"TooltipText"));
    }
    hstring ProxyGroupModel::Type() const
    {
        return m_type;
    }
    void ProxyGroupModel::Type(hstring const &value)
    {
        m_type = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Type"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"IsManualGroup"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"IsSubscription"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"DisplayType"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"DisplayTypeIcon"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"TooltipText"));
    }
    bool ProxyGroupModel::IsManualGroup() const
    {
        return m_type == L"manual";
    }
    bool ProxyGroupModel::IsSubscription() const
    {
        return m_type == L"subscription";
    }
    hstring ProxyGroupModel::DisplayType() const
    {
        if (IsManualGroup())
        {
            return L"Local";
        }
        if (IsSubscription())
        {
            return L"Subscription";
        }
        return m_type;
    }
    hstring ProxyGroupModel::DisplayTypeIcon() const
    {
        if (m_type == L"subscription")
        {
            return L"\uE8F7";
        }
        return L"\uE8B7";
    }
    hstring ProxyGroupModel::TooltipText() const
    {
        if (IsManualGroup())
        {
            return m_name + L" (Local)";
        }
        else if (IsSubscription())
        {
            return m_name + L" (Subscription)";
        }
        return m_name + L" (" + m_type + L")";
    }
    hstring ProxyGroupModel::SubscriptionUrl() const
    {
        return m_subscriptionUrl;
    }
    hstring ProxyGroupModel::SubscriptionUploadUsed() const
    {
        return m_subscriptionUploadUsed;
    }
    hstring ProxyGroupModel::SubscriptionDownloadUsed() const
    {
        return m_subscriptionDownloadUsed;
    }
    hstring ProxyGroupModel::SubscriptionTotalUsed() const
    {
        return m_subscriptionTotalUsed;
    }
    double ProxyGroupModel::SubscriptionPercentUsed() const
    {
        return m_subscriptionPercentUsed;
    }
    bool ProxyGroupModel::SubscriptionHasDataUsage() const
    {
        return m_subscriptionHasDataUsage;
    }
    hstring ProxyGroupModel::SubscriptionBytesTotal() const
    {
        return m_subscriptionBytesTotal;
    }
    hstring ProxyGroupModel::SubscriptionRetrievedAt() const
    {
        return m_subscriptionRetrievedAt;
    }
    hstring ProxyGroupModel::SubscriptionExpireAt() const
    {
        return m_subscriptionExpireAt;
    }
    void ProxyGroupModel::AttachSubscriptionInfo(FfiProxyGroupSubscription const &subscription)
    {
        m_subscriptionUrl = to_hstring(subscription.url);
        m_subscriptionUploadUsed = m_subscriptionDownloadUsed = m_subscriptionBytesTotal = m_subscriptionRetrievedAt =
            L"";
        if (subscription.upload_bytes_used.has_value())
        {
            m_subscriptionUploadUsed = HumanizeByte(subscription.upload_bytes_used.value());
        }
        else
        {
            m_subscriptionUploadUsed = L"";
        }
        if (subscription.download_bytes_used.has_value())
        {
            m_subscriptionDownloadUsed = HumanizeByte(subscription.download_bytes_used.value());
        }
        else
        {
            m_subscriptionDownloadUsed = L"";
        }
        if (subscription.bytes_total.has_value())
        {
            m_subscriptionBytesTotal = HumanizeByte(subscription.bytes_total.value());
        }
        else
        {
            m_subscriptionBytesTotal = L"";
        }
        if (subscription.upload_bytes_used.has_value() && subscription.download_bytes_used.has_value() &&
            subscription.bytes_total.has_value())
        {
            m_subscriptionHasDataUsage = true;
            m_subscriptionTotalUsed =
                HumanizeByte(subscription.upload_bytes_used.value() + subscription.download_bytes_used.value());
            m_subscriptionPercentUsed =
                (subscription.upload_bytes_used.value() + subscription.download_bytes_used.value()) /
                static_cast<double>(subscription.bytes_total.value()) * 100;
        }
        else
        {
            m_subscriptionHasDataUsage = false;
            m_subscriptionTotalUsed = L"";
            m_subscriptionPercentUsed = 0;
        }
        if (subscription.retrieved_at.has_value())
        {
            m_subscriptionRetrievedAt = FormatNaiveDateTime(subscription.retrieved_at.value().c_str());
        }
        else
        {
            m_subscriptionRetrievedAt = L"Never";
        }
        if (subscription.expires_at.has_value())
        {
            m_subscriptionExpireAt = FormatNaiveDateTime(subscription.expires_at.value().c_str());
        }
        else
        {
            m_subscriptionExpireAt = L"";
        }
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionUrl"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionUploadUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionDownloadUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionTotalUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionPercentUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionHasDataUsage"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionBytesTotal"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionRetrievedAt"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionExpireAt"));
    }

    IObservableVector<ProxyModel> ProxyGroupModel::Proxies() const
    {
        return m_proxies;
    }
    void ProxyGroupModel::Proxies(IObservableVector<ProxyModel> const &value)
    {
        m_proxies = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Proxies"));
    }

    event_token ProxyGroupModel::PropertyChanged(
        Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProxyGroupModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
