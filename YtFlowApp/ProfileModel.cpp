#include "pch.h"
#include "ProfileModel.h"
#include "ProfileModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    winrt::event_token ProfileModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ProfileModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    hstring ProfileModel::Name()
    {
        return m_name;
    }
    void ProfileModel::Name(hstring const &value)
    {
        m_name = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Title"});
    }
    uint32_t ProfileModel::Id()
    {
        return m_id;
    }
    void ProfileModel::Id(uint32_t const &value)
    {
        m_id = value;
    }
}
