#pragma once

#include "HomePage.g.h"

#include "CoreRpc.h"

namespace winrt::YtFlowApp::implementation
{
    constexpr wchar_t YTFLOW_CORE_ERROR_LOAD[23] = L"YTFLOW_CORE_ERROR_LOAD";
    struct HomePage : HomePageT<HomePage>
    {
        struct WidgetHandle
        {
            weak_ref<IHomeWidget> widget;
            std::shared_ptr<std::vector<uint8_t>> info;
        };

        using RequestSender =
            std::function<concurrency::task<std::vector<uint8_t>>(std::string_view, std::vector<uint8_t>)>;

        HomePage();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        static Windows::Foundation::IAsyncAction EnsureDatabase();

        void OnConnectRequested(Windows::Foundation::IInspectable const &sender, HomeProfileControl const &control);
        void OnEditRequested(Windows::Foundation::IInspectable const &sender, HomeProfileControl const &control);
        fire_and_forget OnDeleteRequested(Windows::Foundation::IInspectable const &sender,
                                          HomeProfileControl const &control);
        void ConnectCancelButton_Click(Windows::Foundation::IInspectable const &sender,
                                       Windows::UI::Xaml::RoutedEventArgs const &e);
        void DisconnectButton_Click(Windows::Foundation::IInspectable const &sender,
                                    Windows::UI::Xaml::RoutedEventArgs const &e);
        void CreateProfileButton_Click(Windows::Foundation::IInspectable const &sender,
                                       Windows::UI::Xaml::RoutedEventArgs const &e);

        Collections::IObservableVector<YtFlowApp::ProfileModel> Profiles() const;
        RequestSender MakeRequestSender(uint32_t pluginId);

      private:
        static inline Windows::Storage::ApplicationData appData{Windows::Storage::ApplicationData::Current()};

        static IAsyncAction connectToProfile(uint32_t id);

        void SubscribeRefreshPluginStatus();
        std::optional<WidgetHandle> CreateWidgetHandle(RpcPluginInfo const &info);

        rxcpp::composite_subscription m_connStatusChangeSubscription$;
        rxcpp::composite_subscription m_refreshPluginStatus$;
        rxcpp::subjects::subject<bool> m_triggerInfoUpdate$;
        std::map<uint32_t, WidgetHandle> m_widgets;
        Collections::IObservableVector<YtFlowApp::ProfileModel> m_profiles{nullptr};
        IAsyncAction m_vpnTask{nullptr};
        std::atomic<std::shared_ptr<CoreRpc>> m_rpc;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct HomePage : HomePageT<HomePage, implementation::HomePage>
    {
    };
}
