#pragma once

#include "SwitchHomeWidget.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct SwitchHomeWidget : SwitchHomeWidgetT<SwitchHomeWidget>
    {
        using RequestSender =
            std::function<concurrency::task<std::vector<uint8_t>>(std::string_view, std::vector<uint8_t>)>;
        struct SwitchChoice
        {
            std::string name;
            std::string description;
        };
        struct SwitchInfo
        {
            std::vector<SwitchChoice> choices;
            uint32_t current{};
        };
        SwitchHomeWidget();
        SwitchHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo,
                         RequestSender sendRequest);

        fire_and_forget ChoiceToggleButton_Checked(Windows::Foundation::IInspectable const &sender,
                                      Windows::UI::Xaml::RoutedEventArgs const &e);
        void ChoiceToggleButton_Unchecked(Windows::Foundation::IInspectable const &sender,
                                      Windows::UI::Xaml::RoutedEventArgs const &e);

        void UpdateInfo();

      private:
        std::shared_ptr<std::vector<uint8_t>> m_sharedInfo;
        RequestSender m_sendRequest;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SwitchHomeWidget::SwitchChoice, name, description);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SwitchHomeWidget::SwitchInfo, choices, current);
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct SwitchHomeWidget : SwitchHomeWidgetT<SwitchHomeWidget, implementation::SwitchHomeWidget>
    {
    };
}
