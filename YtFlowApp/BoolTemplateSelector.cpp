#include "pch.h"
#include "BoolTemplateSelector.h"
#include "BoolTemplateSelector.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    bool BoolTemplateSelector::Value() const noexcept
    {
        return m_value;
    }
    void BoolTemplateSelector::Value(bool value) noexcept
    {
        m_value = value;
    }

    Windows::UI::Xaml::DataTemplate BoolTemplateSelector::TrueTemplate()
    {
        return m_trueTemplate;
    }
    void BoolTemplateSelector::TrueTemplate(Windows::UI::Xaml::DataTemplate const &value)
    {
        m_trueTemplate = value;
    }

    Windows::UI::Xaml::DataTemplate BoolTemplateSelector::FalseTemplate()
    {
        return m_falseTemplate;
    }
    void BoolTemplateSelector::FalseTemplate(Windows::UI::Xaml::DataTemplate const &value)
    {
        m_falseTemplate = value;
    }

    Windows::UI::Xaml::DataTemplate BoolTemplateSelector::SelectTemplateCore(IInspectable const &item)
    {
        return Select(item);
    }
    Windows::UI::Xaml::DataTemplate BoolTemplateSelector::SelectTemplateCore(
        IInspectable const &item, Windows::UI::Xaml::DependencyObject const & /* container */)
    {
        return Select(item);
    }
    Windows::UI::Xaml::DataTemplate BoolTemplateSelector::Select(IInspectable const &)
    {
        if (m_value)
        {
            return m_trueTemplate;
        }
        return m_falseTemplate;
    }
}
