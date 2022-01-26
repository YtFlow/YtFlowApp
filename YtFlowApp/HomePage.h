﻿#pragma once

#include "HomePage.g.h"
#include <rxcpp/rx.hpp>

namespace winrt::YtFlowApp::implementation
{
    constexpr wchar_t YTFLOW_CORE_ERROR_LOAD[23] = L"YTFLOW_CORE_ERROR_LOAD";
    struct HomePage : HomePageT<HomePage>
    {
        HomePage();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        static Windows::Foundation::IAsyncAction EnsureDatabase();

        void OnConnectRequested(Windows::Foundation::IInspectable const &sender,
                                YtFlowApp::HomeProfileControl const &control);
        void OnEditRequested(Windows::Foundation::IInspectable const &sender,
                                YtFlowApp::HomeProfileControl const &control);
        fire_and_forget OnDeleteRequested(Windows::Foundation::IInspectable const &sender,
                                YtFlowApp::HomeProfileControl const &control);
        void ConnectCancelButton_Click(Windows::Foundation::IInspectable const &sender,
                                       Windows::UI::Xaml::RoutedEventArgs const &e);
        void DisconnectButton_Click(Windows::Foundation::IInspectable const &sender,
                                    Windows::UI::Xaml::RoutedEventArgs const &e);
        void CreateProfileButton_Click(Windows::Foundation::IInspectable const &sender,
                                    Windows::UI::Xaml::RoutedEventArgs const &e);

        Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProfileModel> Profiles() const;

      private:
        static inline Windows::Storage::ApplicationData appData{Windows::Storage::ApplicationData::Current()};

        static Windows::Foundation::IAsyncAction connectToProfile(uint32_t id);

        rxcpp::composite_subscription m_connStatusChangeSubscription;
        winrt::Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProfileModel> m_profiles{nullptr};
        Windows::Foundation::IAsyncAction m_vpnTask{nullptr};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct HomePage : HomePageT<HomePage, implementation::HomePage>
    {
    };
}
