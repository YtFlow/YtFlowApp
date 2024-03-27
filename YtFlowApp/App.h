#pragma once
#include "App.xaml.g.h"

#include "BoolTemplateSelector.h"
#include "BoolToFontWeightConverter.h"
#include "EditPluginTreeViewItemTemplateSelector.h"
#include "IdentityConverter.h"
#include "PluginTypeToDescConverter.h"
#include "ProfileModel.h"
#include "ProxyLegModel.h"
#include "SplitRoutingModeToDescConverter.h"

namespace winrt::YtFlowApp::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const &);
        void OnSuspending(IInspectable const &, Windows::ApplicationModel::SuspendingEventArgs const &);
        void OnNavigationFailed(IInspectable const &, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const &);
    };
}
