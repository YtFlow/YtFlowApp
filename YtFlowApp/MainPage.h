#pragma once

#include "MainPage.g.h"

using namespace winrt::Windows::ApplicationModel::Core;
using winrt::Windows::UI::Core::WindowActivatedEventArgs;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void Page_Loaded(winrt::Windows::Foundation::IInspectable const &sender,
                         winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void NavigationViewControl_DisplayModeChanged(muxc::NavigationView const &sender,
                                                      muxc::NavigationViewDisplayModeChangedEventArgs const &args);
        void CoreTitleBar_IsVisibleChanged(CoreApplicationViewTitleBar const &sender, IInspectable const &args);
        void Current_Activated(IInspectable const &sender, WindowActivatedEventArgs const &args);
        void NavigationViewControl_Loaded(winrt::Windows::Foundation::IInspectable const &sender,
                                          winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void NavigationViewControl_Unloaded(winrt::Windows::Foundation::IInspectable const &sender,
                                            winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void NavView_SelectionChanged(muxc::NavigationView const & /* sender */,
                                      muxc::NavigationViewSelectionChangedEventArgs const &args);
        void NavView_Navigate(std::wstring navItemTag,
                              Windows::UI::Xaml::Media::Animation::NavigationTransitionInfo const &transitionInfo);
        void NavView_BackRequested(muxc::NavigationView const & /* sender */,
                                   muxc::NavigationViewBackRequestedEventArgs const & /* args */);
        void CoreDispatcher_AcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher const & /* sender */,
                                                    Windows::UI::Core::AcceleratorKeyEventArgs const &args);
        void CoreWindow_PointerPressed(Windows::UI::Core::CoreWindow const & /* sender */,
                                       Windows::UI::Core::PointerEventArgs const &args);
        void System_BackRequested(Windows::Foundation::IInspectable const & /* sender */,
                                  Windows::UI::Core::BackRequestedEventArgs const &args);
        bool TryGoBack();
        void ContentFrame_Navigated(winrt::Windows::Foundation::IInspectable const &sender,
                                    winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs const &e);

      private:
        // Vector of std::pair holding the Navigation Tag and the relative Navigation Page.
        static std::vector<std::pair<std::wstring, Windows::UI::Xaml::Interop::TypeName>> m_pages;

        void SubscribeConnectionChanges();
        void UnsubscribeConnectionChanges();

        rxcpp::composite_subscription m_connStatusChangeSubscription;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
