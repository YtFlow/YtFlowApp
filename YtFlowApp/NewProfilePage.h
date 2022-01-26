#pragma once

#include "NewProfilePage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct NewProfilePage : NewProfilePageT<NewProfilePage>
    {
        NewProfilePage();
        fire_and_forget SaveButton_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void OutboundTypeButton_Checked(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Loaded(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void Page_Unloaded(IInspectable const &sender, winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void NewProfileNameText_TextChanged(IInspectable const &sender,
                                            Windows::UI::Xaml::Controls::TextChangedEventArgs const &e);

      private:
        event_token m_ssCheckedToken, m_trojanCheckedToken, m_httpCheckedToken;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct NewProfilePage : NewProfilePageT<NewProfilePage, implementation::NewProfilePage>
    {
    };
}
