#include "pch.h"
#include "ProxyModel.h"
#include "ProxyModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;

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
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Name"));
    }
    com_array<uint8_t> ProxyModel::Proxy() const
    {
        return com_array(m_proxy);
    }
    void ProxyModel::Proxy(array_view<uint8_t const> value)
    {
        m_proxy = std::vector(value.begin(), value.end());
        m_propertyChanged(*this, PropertyChangedEventArgs(L"Proxy"));
    }
    uint16_t ProxyModel::ProxyVersion() const
    {
        return m_proxyVersion;
    }
    void ProxyModel::ProxyVersion(uint16_t value)
    {
        m_proxyVersion = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"ProxyVersion"));
    }
    winrt::event_token ProxyModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProxyModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
