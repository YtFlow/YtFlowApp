#include "pch.h"
#include "FirstTimePage.h"
#if __has_include("FirstTimePage.g.cpp")
#include "FirstTimePage.g.cpp"
#endif

#include "ConnectionState.h"
#include "HomePage.h"
#include "UI.h"

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
        if (m_currentActivated)
        {
            Window::Current().Activated(m_currentActivated);
        }
    }
    fire_and_forget FirstTimePage::Current_Activated(IInspectable const &, WindowActivatedEventArgs const &args)
    {
        try
        {
            if (args.WindowActivationState() == CoreWindowActivationState::Deactivated)
            {
                co_return;
            }

            auto const profile = co_await ConnectionState::GetInstalledVpnProfile();
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
        catch (...)
        {
            NotifyException(L"FirstTimePage activated");
        }
    }
}
