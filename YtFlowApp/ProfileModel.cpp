#include "pch.h"
#include "ProfileModel.h"
#include "ProfileModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    event_token ProfileModel::PropertyChanged(
        Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProfileModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    uint32_t ProfileModel::Id()
    {
        return m_id;
    }
    void ProfileModel::Id(uint32_t const &value)
    {
        m_id = value;
    }
    hstring ProfileModel::Name()
    {
        return m_name;
    }
    void ProfileModel::Name(hstring const &value)
    {
        m_name = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Name"});
    }
    hstring ProfileModel::Locale()
    {
        return m_locale;
    }
    void ProfileModel::Locale(hstring const &value)
    {
        m_locale = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Locale"});
    }
}
