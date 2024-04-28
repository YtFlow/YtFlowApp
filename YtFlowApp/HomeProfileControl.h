#pragma once

#include "HomeProfileControl.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct HomeProfileControl : HomeProfileControlT<HomeProfileControl>
    {
        HomeProfileControl();

        YtFlowApp::ProfileModel Profile();
        void Profile(YtFlowApp::ProfileModel const &value);
        event_token ConnectRequested(Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler);
        void ConnectRequested(event_token const &token) noexcept;
        event_token EditRequested(Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler);
        void EditRequested(event_token const &token) noexcept;
        event_token ExportRequested(Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler);
        void ExportRequested(event_token const &token) noexcept;
        event_token DeleteRequested(Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl> const &handler);
        void DeleteRequested(event_token const &token) noexcept;

        void ConnectButton_Click(Windows::Foundation::IInspectable const &sender,
                                 Windows::UI::Xaml::RoutedEventArgs const &e);
        void EditButton_Click(Windows::Foundation::IInspectable const &sender,
                              Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const &e);
        void EditButton_Click(Windows::Foundation::IInspectable const &sender,
                              Windows::UI::Xaml::RoutedEventArgs const &e);
        void ExportButton_Click(Windows::Foundation::IInspectable const &sender,
                                Windows::UI::Xaml::RoutedEventArgs const &e);
        void DeleteButton_Click(Windows::Foundation::IInspectable const &sender,
                                Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        YtFlowApp::ProfileModel m_profile;
        event<Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl>> m_connectRequested;
        event<Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl>> m_editRequested;
        event<Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl>> m_exportRequested;
        event<Windows::Foundation::EventHandler<YtFlowApp::HomeProfileControl>> m_deleteRequested;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct HomeProfileControl : HomeProfileControlT<HomeProfileControl, implementation::HomeProfileControl>
    {
    };
}
