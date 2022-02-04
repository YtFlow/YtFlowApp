#pragma once

#include "RawEditorPage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct RawEditorPage : RawEditorPageT<RawEditorPage>
    {
        RawEditorPage();

        YtFlowApp::EditPluginModel Model();
        void Model(YtFlowApp::EditPluginModel const &value);

        void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void ParamEdit_LostFocus(Windows::Foundation::IInspectable const &sender,
                                 Windows::UI::Xaml::RoutedEventArgs const &e);
        void ParamEdit_TextChanged(Windows::Foundation::IInspectable const &sender,
                                   Windows::UI::Xaml::RoutedEventArgs const &e);
        void ResetButton_Click(Windows::Foundation::IInspectable const &sender,
                               Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget SaveButton_Click(Windows::Foundation::IInspectable const &sender,
                               Windows::UI::Xaml::RoutedEventArgs const &e);

        Windows::UI::Xaml::Media::SolidColorBrush PluginNameColor(bool hasNamingConflict);

      private:
        YtFlowApp::EditPluginModel m_model;
        event_token m_paramEditTextChangedToken;
        uint8_t m_paramEditTextChangedStage{0};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct RawEditorPage : RawEditorPageT<RawEditorPage, implementation::RawEditorPage>
    {
    };
}
