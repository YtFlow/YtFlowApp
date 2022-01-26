#include "pch.h"
#include "HomeProfileControl.h"
#if __has_include("HomeProfileControl.g.cpp")
#include "HomeProfileControl.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    HomeProfileControl::HomeProfileControl()
    {
        InitializeComponent();
    }

    YtFlowApp::ProfileModel HomeProfileControl::Profile()
    {
        return m_profile;
    }
    void HomeProfileControl::Profile(winrt::YtFlowApp::ProfileModel const &value)
    {
        m_profile = value;
        Bindings->Update();
    }
    winrt::event_token HomeProfileControl::ConnectRequested(
        winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_connectRequested.add(handler);
    }
    void HomeProfileControl::ConnectRequested(winrt::event_token const &token) noexcept
    {
        m_connectRequested.remove(token);
    }
    winrt::event_token HomeProfileControl::EditRequested(
        winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_editRequested.add(handler);
    }
    void HomeProfileControl::EditRequested(winrt::event_token const &token) noexcept
    {
        m_editRequested.remove(token);
    }
    winrt::event_token HomeProfileControl::DeleteRequested(
        winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_deleteRequested.add(handler);
    }
    void HomeProfileControl::DeleteRequested(winrt::event_token const &token) noexcept
    {
        m_deleteRequested.remove(token);
    }

    void HomeProfileControl::ConnectButton_Click(IInspectable const & /* sender */,
                                                 Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        m_connectRequested(*this, *this);
    }
    void HomeProfileControl::EditButton_Click(IInspectable const & /* sender */,
                                              Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const & /* e */)
    {
        m_editRequested(*this, *this);
    }
    void HomeProfileControl::DeleteButton_Click(IInspectable const & /* sender */,
                                                Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        m_deleteRequested(*this, *this);
    }

}
