#pragma once
#include "BoolTemplateSelector.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct BoolTemplateSelector : BoolTemplateSelectorT<BoolTemplateSelector>
    {
        BoolTemplateSelector() = default;

        bool Value() const noexcept;
        void Value(bool value) noexcept;
        Windows::UI::Xaml::DataTemplate TrueTemplate();
        void TrueTemplate(Windows::UI::Xaml::DataTemplate const &value);
        Windows::UI::Xaml::DataTemplate FalseTemplate();
        void FalseTemplate(Windows::UI::Xaml::DataTemplate const &value);

        Windows::UI::Xaml::DataTemplate SelectTemplateCore(IInspectable const &item);
        Windows::UI::Xaml::DataTemplate SelectTemplateCore(IInspectable const &item,
                                                           Windows::UI::Xaml::DependencyObject const &container);

      private:
        Windows::UI::Xaml::DataTemplate Select(IInspectable const &item);

        bool m_value{false};
        Windows::UI::Xaml::DataTemplate m_trueTemplate{nullptr};
        Windows::UI::Xaml::DataTemplate m_falseTemplate{nullptr};
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct BoolTemplateSelector : BoolTemplateSelectorT<BoolTemplateSelector, implementation::BoolTemplateSelector>
    {
    };
}
