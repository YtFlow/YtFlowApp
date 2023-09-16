#pragma once
#include "DynOutboundProxyModel.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct DynOutboundProxyModel : DynOutboundProxyModelT<DynOutboundProxyModel>
    {
        DynOutboundProxyModel() = default;
        DynOutboundProxyModel(uint32_t idx, hstring name, hstring groupName)
            : m_idx(idx), m_name(std::move(name)), m_groupName(std::move(groupName))
        {
        }

        hstring Name() const;
        hstring GroupName() const;
        uint32_t Idx() const;
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

      private:
        uint32_t m_idx;
        hstring m_name;
        hstring m_groupName;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct DynOutboundProxyModel : DynOutboundProxyModelT<DynOutboundProxyModel, implementation::DynOutboundProxyModel>
    {
    };
}
