#include "pch.h"
#include "HomeProfileControl.h"
#if __has_include("HomeProfileControl.g.cpp")
#include "HomeProfileControl.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using Windows::Foundation::Metadata::ApiInformation;
using Windows::UI::Xaml::Controls::Symbol;
using Windows::UI::Xaml::Controls::SymbolIcon;

namespace winrt::YtFlowApp::implementation
{
    HomeProfileControl::HomeProfileControl()
    {
    }

    ProfileModel HomeProfileControl::Profile()
    {
        return m_profile;
    }
    void HomeProfileControl::Profile(ProfileModel const &value)
    {
        m_profile = value;
        Bindings->Update();
    }

    event_token HomeProfileControl::ConnectRequested(
        Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_connectRequested.add(handler);
    }
    void HomeProfileControl::ConnectRequested(event_token const &token) noexcept
    {
        m_connectRequested.remove(token);
    }

    event_token HomeProfileControl::EditRequested(
        Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_editRequested.add(handler);
    }
    void HomeProfileControl::EditRequested(event_token const &token) noexcept
    {
        m_editRequested.remove(token);
    }

    event_token HomeProfileControl::ExportRequested(
        Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_exportRequested.add(handler);
    }
    void HomeProfileControl::ExportRequested(event_token const &token) noexcept
    {
        m_exportRequested.remove(token);
    }

    event_token HomeProfileControl::DeleteRequested(
        Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler)
    {
        return m_deleteRequested.add(handler);
    }
    void HomeProfileControl::DeleteRequested(event_token const &token) noexcept
    {
        m_deleteRequested.remove(token);
    }

    void HomeProfileControl::UserControl_Loaded(Windows::Foundation::IInspectable const &,
                                                Windows::UI::Xaml::RoutedEventArgs const &)
    {
        if (ApiInformation::IsEnumNamedValuePresent(L"Windows.UI.Xaml.Controls.Symbol", L"Share"))
        {
            ExportButton().Icon(SymbolIcon(Symbol::Share));
        }
    }

    void HomeProfileControl::ConnectButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        m_connectRequested(*this, *this);
    }
    void HomeProfileControl::EditButton_Click(IInspectable const & /* sender */,
                                              Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const & /* e */)
    {
        m_editRequested(*this, *this);
    }
    void HomeProfileControl::EditButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        m_editRequested(*this, *this);
    }
    void HomeProfileControl::ExportButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        m_exportRequested(*this, *this);
    }
    void HomeProfileControl::DeleteButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        m_deleteRequested(*this, *this);
    }

}
