#pragma once
#include "EditPluginTreeViewItemTemplateSelector.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditPluginTreeViewItemTemplateSelector
        : EditPluginTreeViewItemTemplateSelectorT<EditPluginTreeViewItemTemplateSelector>
    {
        EditPluginTreeViewItemTemplateSelector() = default;

        Windows::UI::Xaml::DataTemplate CategoryTemplate();
        void CategoryTemplate(Windows::UI::Xaml::DataTemplate const &value);
        Windows::UI::Xaml::DataTemplate PluginTemplate();
        void PluginTemplate(Windows::UI::Xaml::DataTemplate const &value);

        Windows::UI::Xaml::DataTemplate SelectTemplateCore(IInspectable const &item);
        Windows::UI::Xaml::DataTemplate SelectTemplateCore(IInspectable const &item,
                                                           Windows::UI::Xaml::DependencyObject const &container);

      private:
        Windows::UI::Xaml::DataTemplate Select(IInspectable const &item);

        Windows::UI::Xaml::DataTemplate m_categoryTemplate{nullptr};
        Windows::UI::Xaml::DataTemplate m_pluginTemplate{nullptr};
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct EditPluginTreeViewItemTemplateSelector
        : EditPluginTreeViewItemTemplateSelectorT<EditPluginTreeViewItemTemplateSelector,
                                                  implementation::EditPluginTreeViewItemTemplateSelector>
    {
    };
}
