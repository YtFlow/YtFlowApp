#pragma once

#include <winrt/Windows.Foundation.h>

using namespace winrt::Windows::Foundation;

namespace winrt::YtFlowApp::implementation
{
    void NotifyUser(hstring msg, hstring title = {}, Windows::UI::Core::CoreDispatcher inputDispatcher = {nullptr});
    void NotifyUser(char const *msg, hstring title);
    void NotifyException(std::wstring_view context);

    hstring HumanizeByte(uint64_t num);
    hstring HumanizeByteSpeed(uint64_t num);
    hstring FormatNaiveDateTime(char const *dateStr);
}
