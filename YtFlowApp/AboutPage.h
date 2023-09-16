#pragma once

#include "AboutPage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct AboutPage : AboutPageT<AboutPage>
    {
        AboutPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void Page_Loaded(Windows::Foundation::IInspectable const &sender,
                         Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget LicenseButton_Click(Windows::Foundation::IInspectable const &sender,
                                            Windows::UI::Xaml::RoutedEventArgs const &e);
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage>
    {
    };
}
