#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

#include "ConnectionState.h"
#include "WinrtScheduler.h"

#include <winrt/Windows.System.Profile.h>
#include <winrt/Windows.UI.ViewManagement.h>

using namespace std::chrono_literals;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Media;
using Windows::UI::Colors;
using Windows::UI::ViewManagement::ApplicationView;
using Controls::TextBlock;
namespace muxc = Microsoft::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    std::vector<std::pair<std::wstring, Interop::TypeName>> MainPage::m_pages = {
        {L"home", xaml_typename<HomePage>()},
        {L"library", xaml_typename<LibraryPage>()},
        {L"about", xaml_typename<AboutPage>()}};

    MainPage::MainPage()
    {
        InitializeComponent();

        // Hide default title bar.
        const auto &coreTitleBar = CoreApplication::GetCurrentView().TitleBar();

        // Set XAML element as a draggable region.
        Window::Current().SetTitleBar(AppTitleBar());

        // Register a handler for when the title bar visibility changes.
        // For example, when the title bar is invoked in full screen mode.
        coreTitleBar.IsVisibleChanged({this, &MainPage::CoreTitleBar_IsVisibleChanged});

        // Register a handler for when the window changes focus
        Window::Current().Activated({this, &MainPage::Current_Activated});
    }

    void MainPage::Page_Loaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        if (Windows::System::Profile::AnalyticsInfo::VersionInfo().DeviceFamily() == L"Windows.Mobile")
        {
            Thickness margin{};
            margin.Top = -80;
            ContentFrame().Margin(margin);
        }
    }

    void MainPage::NavigationViewControl_DisplayModeChanged(
        muxc::NavigationView const &sender, muxc::NavigationViewDisplayModeChangedEventArgs const & /* args */)
    {
        const auto &coreTitleBar = CoreApplication::GetCurrentView().TitleBar();
        const auto &titleBar = ApplicationView::GetForCurrentView().TitleBar();

        // Set the TitleBar margin dependent on NavigationView display mode
        if (sender.PaneDisplayMode() == muxc::NavigationViewPaneDisplayMode::Top)
        {
            coreTitleBar.ExtendViewIntoTitleBar(true);
            titleBar.ButtonBackgroundColor(Colors::Transparent());
            titleBar.ButtonInactiveBackgroundColor(Colors::Transparent());
        }
        else if (sender.DisplayMode() == muxc::NavigationViewDisplayMode::Minimal)
        {
            coreTitleBar.ExtendViewIntoTitleBar(false);
            titleBar.ButtonBackgroundColor({});
            titleBar.ButtonInactiveBackgroundColor({});
        }
        else
        {
            coreTitleBar.ExtendViewIntoTitleBar(false);
            titleBar.ButtonBackgroundColor({});
            titleBar.ButtonInactiveBackgroundColor({});
        }
    }
    void MainPage::CoreTitleBar_IsVisibleChanged(CoreApplicationViewTitleBar const &sender,
                                                 IInspectable const & /* args */)
    {
        if (sender.IsVisible())
        {
            AppTitleBar().Visibility(Visibility::Visible);
        }
        else
        {
            AppTitleBar().Visibility(Visibility::Collapsed);
        }
    }
    void MainPage::Current_Activated(IInspectable const & /* sender */, WindowActivatedEventArgs const &e)
    {
        auto defaultForegroundBrush =
            Application::Current().Resources().Lookup(box_value(L"TextFillColorPrimaryBrush")).as<SolidColorBrush>();
        auto inactiveForegroundBrush =
            Application::Current().Resources().Lookup(box_value(L"TextFillColorDisabledBrush")).as<SolidColorBrush>();

        if (e.WindowActivationState() == Windows::UI::Core::CoreWindowActivationState::Deactivated)
        {
            AppTitle().Foreground(inactiveForegroundBrush);
        }
        else
        {
            AppTitle().Foreground(defaultForegroundBrush);
        }
    }

    void MainPage::NavigationViewControl_Loaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        // NavView doesn't load any page by default, so load home page.
        NavigationViewControl().SelectedItem(NavigationViewControl().MenuItems().GetAt(0));
        // If navigation occurs on SelectionChanged, then this isn't needed.
        // Because we use ItemInvoked to navigate, we need to call Navigate
        // here to load the home page.
        // TODO: link error?
        // NavView_Navigate(L"home", Windows::UI::Xaml::Media::Animation::EntranceNavigationTransitionInfo());

        // Listen to the window directly so the app responds
        // to accelerator keys regardless of which element has focus.
        Window::Current().CoreWindow().Dispatcher().AcceleratorKeyActivated(
            {this, &MainPage::CoreDispatcher_AcceleratorKeyActivated});

        Window::Current().CoreWindow().PointerPressed(
            {this, &MainPage::CoreWindow_PointerPressed});

        Windows::UI::Core::SystemNavigationManager::GetForCurrentView().BackRequested(
            {this, &MainPage::System_BackRequested});

        SubscribeConnectionChanges();
    }
    void MainPage::NavigationViewControl_Unloaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        UnsubscribeConnectionChanges();
    }

    void MainPage::NavView_SelectionChanged(muxc::NavigationView const & /* sender */,
                                            muxc::NavigationViewSelectionChangedEventArgs const &args)
    {
        if (args.SelectedItemContainer())
        {
            NavView_Navigate(winrt::unbox_value_or<hstring>(args.SelectedItemContainer().Tag(), L"").c_str(),
                             args.RecommendedNavigationTransitionInfo());
        }
    }

    void MainPage::NavView_Navigate(std::wstring navItemTag,
                                    Animation::NavigationTransitionInfo const &transitionInfo)
    {
        Interop::TypeName pageTypeName;
        for (auto &&eachPage : m_pages)
        {
            if (eachPage.first == navItemTag)
            {
                pageTypeName = eachPage.second;
                break;
            }
        }
        // Get the page type before navigation so you can prevent duplicate
        // entries in the backstack.
        Interop::TypeName preNavPageType = ContentFrame().CurrentSourcePageType();

        if (preNavPageType.Name == xaml_typename<FirstTimePage>().Name)
        {
            // Not allowed to skip first time page
            return;
        }

        // Navigate only if the selected page isn't currently loaded.
        if (pageTypeName.Name != L"" && preNavPageType.Name != pageTypeName.Name)
        {
            ContentFrame().Navigate(pageTypeName, nullptr, transitionInfo);
        }
    }

    void MainPage::NavView_BackRequested(muxc::NavigationView const & /* sender */,
                                         muxc::NavigationViewBackRequestedEventArgs const & /* args */)
    {
        TryGoBack();
    }

    void MainPage::CoreDispatcher_AcceleratorKeyActivated(CoreDispatcher const & /* sender */,
                                                          Windows::UI::Core::AcceleratorKeyEventArgs const &args)
    {
        // When Alt+Left are pressed navigate back
        if (args.EventType() == Windows::UI::Core::CoreAcceleratorKeyEventType::SystemKeyDown &&
            args.VirtualKey() == Windows::System::VirtualKey::Left && args.KeyStatus().IsMenuKeyDown && !args.Handled())
        {
            args.Handled(TryGoBack());
        }
    }

    void MainPage::CoreWindow_PointerPressed(Windows::UI::Core::CoreWindow const & /* sender */,
                                             Windows::UI::Core::PointerEventArgs const &args)
    {
        // Handle mouse back button.
        if (args.CurrentPoint().Properties().IsXButton1Pressed())
        {
            args.Handled(TryGoBack());
        }
    }

    void MainPage::System_BackRequested(Windows::Foundation::IInspectable const & /* sender */,
                                        Windows::UI::Core::BackRequestedEventArgs const &args)
    {
        if (!args.Handled())
        {
            args.Handled(TryGoBack());
        }
    }

    bool MainPage::TryGoBack()
    {
        if (!ContentFrame().CanGoBack())
            return false;
        // Don't go back if the nav pane is overlayed.
        if (NavigationViewControl().IsPaneOpen() &&
            (NavigationViewControl().DisplayMode() == muxc::NavigationViewDisplayMode::Compact ||
             NavigationViewControl().DisplayMode() == muxc::NavigationViewDisplayMode::Minimal))
            return false;

        Interop::TypeName preNavPageType = ContentFrame().CurrentSourcePageType();
        if (preNavPageType.Name == xaml_typename<FirstTimePage>().Name)
        {
            // Not allowed to skip first time page
            return false;
        }

        ContentFrame().GoBack();
        return true;
    }

    void MainPage::SubscribeConnectionChanges()
    {
        auto weak = get_weak();
        m_connStatusChangeSubscription =
            rxcpp::observable<>::interval(500ms, rxcpp::observe_on_event_loop())
                .filter([](auto) { return ConnectionState::Instance.has_value(); })
                .first()
                .flat_map([](auto) { return ConnectionState::Instance->ConnectStatusChange$; })
                .observe_on(ObserveOnDispatcher())
                .subscribe([=](auto e) {
                    if (auto self{weak.get()})
                    {
                        TextBlock content{nullptr};
                        switch (e)
                        {
                        case VpnManagementConnectionStatus::Disconnected:
                            self->AppConnectedStatePlaceholder().Content(nullptr);
                            break;
                        case VpnManagementConnectionStatus::Connected:
                            content = std::move(TextBlock{});
                            content.Style(self->AppConnectedStateStyle());
                            self->AppConnectedStatePlaceholder().Content(std::move(content));
                            break;
                        default:
                            break;
                        }
                    }
                });
    }
    void MainPage::UnsubscribeConnectionChanges()
    {
        m_connStatusChangeSubscription.unsubscribe();
    }

    void MainPage::ContentFrame_Navigated(IInspectable const &, Navigation::NavigationEventArgs const &e)
    {
        auto const sourcePageName = std::move(e.SourcePageType().Name);
        std::optional<std::reference_wrapper<std::wstring const>> pageTag;
        for (auto const &[tag, page] : m_pages)
        {
            if (sourcePageName == page.Name)
            {
                pageTag = std::make_optional(std::ref(tag));
                break;
            }
        }
        if (!pageTag.has_value())
        {
            return;
        }
        for (auto const menuObj : NavigationViewControl().MenuItems())
        {
            auto const menuItem = menuObj.try_as<muxc::NavigationViewItem>();
            if (menuItem == nullptr)
            {
                continue;
            }
            if (menuItem.Tag().try_as<hstring>() == pageTag.value())
            {
                NavigationViewControl().SelectedItem(menuObj);
                break;
            }
        }
    }

}
