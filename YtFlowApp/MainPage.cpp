﻿#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "RxDispatcherScheduler.h"
#include <winrt/Windows.UI.ViewManagement.h>

using namespace std::chrono_literals;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using Windows::UI::Colors;
using Windows::UI::ViewManagement::ApplicationView;
using Windows::UI::Xaml::Controls::TextBlock;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
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
        winrt::Windows::UI::Xaml::Window::Current().CoreWindow().Dispatcher().AcceleratorKeyActivated(
            {this, &MainPage::CoreDispatcher_AcceleratorKeyActivated});

        winrt::Windows::UI::Xaml::Window::Current().CoreWindow().PointerPressed(
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
            NavView_Navigate(winrt::unbox_value_or<winrt::hstring>(args.SelectedItemContainer().Tag(), L"").c_str(),
                             args.RecommendedNavigationTransitionInfo());
        }
    }

    void MainPage::NavView_Navigate(std::wstring navItemTag,
                                    Windows::UI::Xaml::Media::Animation::NavigationTransitionInfo const &transitionInfo)
    {
        Windows::UI::Xaml::Interop::TypeName pageTypeName;
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
        Windows::UI::Xaml::Interop::TypeName preNavPageType = ContentFrame().CurrentSourcePageType();

        if (preNavPageType.Name == xaml_typename<YtFlowApp::FirstTimePage>().Name)
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

    void MainPage::CoreDispatcher_AcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher const & /* sender */,
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

        Windows::UI::Xaml::Interop::TypeName preNavPageType = ContentFrame().CurrentSourcePageType();
        if (preNavPageType.Name == xaml_typename<YtFlowApp::FirstTimePage>().Name)
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
}