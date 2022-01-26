#include "pch.h"
#include "Rx.h"

using namespace winrt::Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    rxcpp::observable<bool> ObserveApplicationEnteredBackground()
    {
        static Application app{Application::Current()};
        return Rx::observe_winrt_event<bool>([](auto const cb) { return app.EnteredBackground(cb); },
                                             [](auto const &, auto const &) { return true; },
                                             [](auto const token) { app.EnteredBackground(token); });
    }
    rxcpp::observable<bool> ObserveApplicationLeavingBackground()
    {
        static Application app{Application::Current()};
        return Rx::observe_winrt_event<bool>([](auto const cb) { return app.LeavingBackground(cb); },
                                             [](auto const &, auto const &) { return true; },
                                             [](auto const token) { app.LeavingBackground(token); });
    }
}
