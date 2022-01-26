#pragma once

#include "FirstTimePage.g.h"

using winrt::Windows::UI::Core::WindowActivatedEventArgs;

namespace winrt::YtFlowApp::implementation
{
    struct FirstTimePage : FirstTimePageT<FirstTimePage>
    {
        FirstTimePage();

        void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        fire_and_forget Current_Activated(IInspectable const &sender, WindowActivatedEventArgs const &args);

      private:
        std::optional<event_token> m_currentActivated;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct FirstTimePage : FirstTimePageT<FirstTimePage, implementation::FirstTimePage>
    {
    };
}
