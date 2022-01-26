#pragma once

#include <winrt/Windows.Foundation.h>

using namespace winrt::Windows::Foundation;

namespace winrt::YtFlowApp::implementation
{
    void NotifyUser(hstring msg, hstring title = {});
}
