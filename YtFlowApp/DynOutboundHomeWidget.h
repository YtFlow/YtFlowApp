#pragma once

#include "DynOutboundHomeWidget.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct DynOutboundHomeWidget : DynOutboundHomeWidgetT<DynOutboundHomeWidget>
    {
        using RequestSender =
            std::function<concurrency::task<std::vector<uint8_t>>(std::string_view, std::vector<uint8_t>)>;

        DynOutboundHomeWidget()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
        DynOutboundHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo,
                              RequestSender sendRequest);
        void UserControl_Loaded(winrt::Windows::Foundation::IInspectable const &sender,
                                winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget SelectProxyButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                                winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxySelectionBackButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                            winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget ProxyItem_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                        winrt::Windows::UI::Xaml::RoutedEventArgs const &e);

        void UpdateInfo();

      private:
        hstring m_pluginName;
        std::shared_ptr<std::vector<uint8_t>> m_sharedInfo;
        RequestSender m_sendRequest;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct DynOutboundHomeWidget : DynOutboundHomeWidgetT<DynOutboundHomeWidget, implementation::DynOutboundHomeWidget>
    {
    };
}
