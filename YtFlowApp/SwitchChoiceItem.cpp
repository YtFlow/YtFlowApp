#include "pch.h"
#include "SwitchChoiceItem.h"
#include "SwitchChoiceItem.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    hstring SwitchChoiceItem::Name()
    {
        return m_name;
    }
    void SwitchChoiceItem::Name(hstring const &value)
    {
        m_name = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Name"});
    }
    hstring SwitchChoiceItem::Description()
    {
        return m_desc;
    }
    void SwitchChoiceItem::Description(hstring const &value)
    {
        m_desc = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Description"});
    }
    bool SwitchChoiceItem::IsActive() noexcept
    {
        return m_isActive;
    }
    void SwitchChoiceItem::IsActive(bool isActive)
    {
        m_isActive = isActive;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"IsActive"});
    }

    event_token SwitchChoiceItem::PropertyChanged(
        Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void SwitchChoiceItem::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
