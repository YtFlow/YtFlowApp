#pragma once
#include "App.xaml.g.h"

#include "ProfileModel.h"
#include "EditPluginTreeViewItemTemplateSelector.h"
#include "PluginTypeToDescConverter.h"
#include "BoolToFontWeightConverter.h"

namespace winrt::YtFlowApp::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, Windows::ApplicationModel::SuspendingEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
