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
        m_propertyChanged(*this, PropertyChangedEventArgs(L"DisplayType"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"DisplayTypeIcon"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"TooltipText"));
    }
    bool ProxyGroupModel::IsManualGroup() const
    {
        return m_type == L"manual";
    }
    hstring ProxyGroupModel::DisplayType() const
    {
        if (IsManualGroup())
        {
            return L"Local";
        }
        else if (m_type == L"subscription")
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
        else if (m_type == L"subscription")
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
    hstring ProxyGroupModel::SubscriptionBytesTotal() const
    {
        return m_subscriptionBytesTotal;
    }
    hstring ProxyGroupModel::SubscriptionRetrievedAt() const
    {
        return m_subscriptionRetrievedAt;
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
        if (subscription.download_bytes_used.has_value())
        {
            m_subscriptionDownloadUsed = HumanizeByte(subscription.download_bytes_used.value());
        }
        if (subscription.bytes_total.has_value())
        {
            m_subscriptionBytesTotal = HumanizeByte(subscription.bytes_total.value());
        }
        if (subscription.retrieved_at.has_value())
        {
            m_subscriptionRetrievedAt = to_hstring(subscription.retrieved_at.value());
        }
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionUrl"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionUploadUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionDownloadUsed"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionBytesTotal"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"SubscriptionRetrievedAt"));
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

    winrt::event_token ProxyGroupModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProxyGroupModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
