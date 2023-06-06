#pragma once

#include "NewProfilePage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct NewProfileConfig
    {
        hstring OutboundType;
    };
    struct NewProfilePage : NewProfilePageT<NewProfilePage>
    {
        NewProfilePage();

        void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);

        fire_and_forget SaveButton_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void OutboundTypeButton_Checked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void SplitRoutingTypeButton_Checked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void DynOutboundButton_Unchecked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Loaded(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Unloaded(IInspectable const &sender, winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void NewProfileNameText_TextChanged(IInspectable const &sender,
                                            Windows::UI::Xaml::Controls::TextChangedEventArgs const &e);
        fire_and_forget SelectRulesetButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                                  winrt::Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        static void CreatePresetPlugins(uint32_t profileId, NewProfileConfig config);

        event_token m_dynOutboundCheckedToken, m_ssCheckedToken, m_trojanCheckedToken, m_vmessWsTlsCheckedToken,
            m_httpCheckedToken;
        event_token m_allProxyToken, m_whitelistToken, m_overseasToken;
        hstring m_selectedOutboundType{L"dyn"}, m_selectedSplitRoutingType{L"all"};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct NewProfilePage : NewProfilePageT<NewProfilePage, implementation::NewProfilePage>
    {
    };
}
