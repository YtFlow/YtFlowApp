#include "pch.h"
#include "EditPluginTreeViewItemTemplateSelector.h"
#include "EditPluginTreeViewItemTemplateSelector.g.cpp"

#include "EditPluginModel.h"

namespace winrt::YtFlowApp::implementation
{
    winrt::Windows::UI::Xaml::DataTemplate EditPluginTreeViewItemTemplateSelector::CategoryTemplate()
    {
        return m_categoryTemplate;
    }
    void EditPluginTreeViewItemTemplateSelector::CategoryTemplate(winrt::Windows::UI::Xaml::DataTemplate const &value)
    {
        m_categoryTemplate = value;
    }
    winrt::Windows::UI::Xaml::DataTemplate EditPluginTreeViewItemTemplateSelector::PluginTemplate()
    {
        return m_pluginTemplate;
    }
    void EditPluginTreeViewItemTemplateSelector::PluginTemplate(winrt::Windows::UI::Xaml::DataTemplate const &value)
    {
        m_pluginTemplate = value;
    }

    Windows::UI::Xaml::DataTemplate EditPluginTreeViewItemTemplateSelector::SelectTemplateCore(IInspectable const &item)
    {
        return Select(item);
    }
    Windows::UI::Xaml::DataTemplate EditPluginTreeViewItemTemplateSelector::SelectTemplateCore(
        IInspectable const &item, Windows::UI::Xaml::DependencyObject const & /* container */)
    {
        return Select(item);
    }
    Windows::UI::Xaml::DataTemplate EditPluginTreeViewItemTemplateSelector::Select(IInspectable const &item)
    {
        const auto tvc{item.as<Microsoft::UI::Xaml::Controls::TreeViewNode>().Content()};
        if (tvc.try_as<YtFlowApp::EditPluginModel>() != nullptr)
        {
            return m_pluginTemplate;
        }
        return m_categoryTemplate;
    }
}
