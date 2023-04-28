#include "pch.h"
#include "DynOutboundProxyModel.h"
#include "DynOutboundProxyModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    hstring DynOutboundProxyModel::Name() const
    {
        return m_name;
    }
    hstring DynOutboundProxyModel::GroupName() const
    {
        return m_groupName;
    }
    uint32_t DynOutboundProxyModel::Idx() const
    {
        return m_idx;
    }
    winrt::event_token DynOutboundProxyModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void DynOutboundProxyModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
