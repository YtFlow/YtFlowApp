#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "ResourceManageControl.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct ResourceManageControl : ResourceManageControlT<ResourceManageControl>
    {
        ResourceManageControl() 
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct ResourceManageControl : ResourceManageControlT<ResourceManageControl, implementation::ResourceManageControl>
    {
    };
}
