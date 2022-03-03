#pragma once
#include "SwitchChoiceItem.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct SwitchChoiceItem : SwitchChoiceItemT<SwitchChoiceItem>
    {
        SwitchChoiceItem() = default;

        hstring Name();
        void Name(hstring const &value);
        hstring Description();
        void Description(hstring const &value);
        bool IsActive() noexcept;
        void IsActive(bool isActive);

        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;

      private:
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        hstring m_name;
        hstring m_desc;
        bool m_isActive{};
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct SwitchChoiceItem : SwitchChoiceItemT<SwitchChoiceItem, implementation::SwitchChoiceItem>
    {
    };
}
