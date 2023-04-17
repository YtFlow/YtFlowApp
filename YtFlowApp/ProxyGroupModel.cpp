#include "pch.h"
#include "ProxyGroupModel.h"
#include "ProxyGroupModel.g.cpp"

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
