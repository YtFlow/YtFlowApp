#pragma once

#include "NewProfilePage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct NewProfilePage : NewProfilePageT<NewProfilePage>
    {
        NewProfilePage();

        static nlohmann::json GenPresetDoc();

        void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);

        fire_and_forget SaveButton_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void OutboundTypeButton_Checked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget SplitRoutingModeButtons_SelectionChanged(
            IInspectable const &sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs const &e);
        void DynOutboundButton_Unchecked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Loaded(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Unloaded(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void NewProfileNameText_TextChanged(IInspectable const &sender,
                                            Windows::UI::Xaml::Controls::TextChangedEventArgs const &e);
        fire_and_forget AddRulesetButton_Click(Windows::Foundation::IInspectable const &sender,
                                               Windows::UI::Xaml::RoutedEventArgs const &e);
        void CreateCustomRuleButton_Click(Windows::Foundation::IInspectable const &sender,
                                          Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        void AddListRuleset(hstring name, SplitRoutingRuleDecision match,
                            SplitRoutingRuleDecision unmatch = SplitRoutingRuleDecision::Next);
        void AddRuleRuleset(hstring name, hstring matchRule, SplitRoutingRuleDecision match,
                            SplitRoutingRuleDecision unmatch = SplitRoutingRuleDecision::Next);

        event_token m_dynOutboundCheckedToken, m_ssCheckedToken, m_trojanCheckedToken, m_vmessWsTlsCheckedToken,
            m_httpCheckedToken;
        hstring m_selectedOutboundType{L"dyn"};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct NewProfilePage : NewProfilePageT<NewProfilePage, implementation::NewProfilePage>
    {
    };
}
