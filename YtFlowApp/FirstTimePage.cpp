#include "pch.h"
#include "FirstTimePage.h"
#if __has_include("FirstTimePage.g.cpp")
#include "FirstTimePage.g.cpp"
#endif

#include "HomePage.h"
#include "ConnectionState.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using winrt::Windows::UI::Core::CoreWindowActivationState;

namespace winrt::YtFlowApp::implementation
{
    FirstTimePage::FirstTimePage()
    {
        InitializeComponent();
    }

    void FirstTimePage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &)
    {
        m_currentActivated = Window::Current().Activated({this, &FirstTimePage::Current_Activated});
    }
    void FirstTimePage::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &)
    {
        std::optional<event_token> currentActivated{};
        m_currentActivated.swap(currentActivated);
        if (currentActivated.has_value())
        {
            Window::Current().Activated(*currentActivated);
        }
    }
    fire_and_forget FirstTimePage::Current_Activated(IInspectable const &, WindowActivatedEventArgs const &args)
    {
        if (args.WindowActivationState() == CoreWindowActivationState::Deactivated)
        {
            co_return;
        }

        const auto &profile = co_await ConnectionState::GetInstalledVpnProfile();
        if (profile == nullptr)
        {
            co_return;
        }
        ConnectionState::Instance.emplace(profile);

        if (Frame().CanGoBack())
        {
            Frame().GoBack();
        }
        else
        {
            Frame().Navigate(xaml_typename<YtFlowApp::HomePage>());
        }
    }
}
