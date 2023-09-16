#pragma once

#include "ForwardHomeWidget.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct ForwardStatSnapshot
    {
        uint64_t uplink_written{};
        uint64_t downlink_written{};
        uint32_t tcp_connections{};
        uint32_t udp_sessions{};
    };
    struct ForwardHomeWidget : ForwardHomeWidgetT<ForwardHomeWidget>
    {
        ForwardHomeWidget()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        ForwardHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo);

        void UserControl_Loaded(Windows::Foundation::IInspectable const &sender,
                                Windows::UI::Xaml::RoutedEventArgs const &e);
        void UserControl_Unloaded(Windows::Foundation::IInspectable const &sender,
                                  Windows::UI::Xaml::RoutedEventArgs const &e);

        void UpdateInfo();

      private:
        rxcpp::composite_subscription m_renderStat$;
        std::shared_ptr<std::vector<uint8_t>> m_sharedInfo;
        ForwardStatSnapshot m_lastStat{};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct ForwardHomeWidget : ForwardHomeWidgetT<ForwardHomeWidget, implementation::ForwardHomeWidget>
    {
    };
}
