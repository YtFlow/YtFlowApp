#pragma once

#include "HomeProfileControl.g.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.h"

namespace winrt::YtFlowApp::implementation
{
    struct HomeProfileControl : HomeProfileControlT<HomeProfileControl>
    {
        HomeProfileControl();

        winrt::YtFlowApp::ProfileModel Profile();
        void Profile(winrt::YtFlowApp::ProfileModel const &value);
        winrt::event_token ConnectRequested(
            winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler);
        void ConnectRequested(winrt::event_token const &token) noexcept;
        winrt::event_token EditRequested(
            winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler);
        void EditRequested(winrt::event_token const &token) noexcept;
        winrt::event_token DeleteRequested(
            winrt::Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl> const &handler);
        void DeleteRequested(winrt::event_token const &token) noexcept;

        void ConnectButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                 winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void EditButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                 winrt::Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const &e);
        void EditButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                 winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void DeleteButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                 winrt::Windows::UI::Xaml::RoutedEventArgs const &e);



      private:
        winrt::YtFlowApp::ProfileModel m_profile;
        winrt::event<Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl>> m_connectRequested;
        winrt::event<Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl>> m_editRequested;
        winrt::event<Windows::Foundation::EventHandler<winrt::YtFlowApp::HomeProfileControl>> m_deleteRequested;

    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct HomeProfileControl : HomeProfileControlT<HomeProfileControl, implementation::HomeProfileControl>
    {
    };
}
