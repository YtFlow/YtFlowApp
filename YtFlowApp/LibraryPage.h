#pragma once

#include "LibraryPage.g.h"

#include "AssetModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage>
    {
        LibraryPage();

        YtFlowApp::AssetModel Model() const;
        fire_and_forget LoadModel();
        void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);
        fire_and_forget ProxyGroupItemDelete_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                                   winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupItemRename_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                        winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget RenameProxyGroupItem(YtFlowApp::ProxyGroupModel const &item);
        fire_and_forget CreateProxyGroupButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                                     winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupItem_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                  winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupProxyList_SelectionChanged(
            winrt::Windows::Foundation::IInspectable const &sender,
            winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const &e);
        fire_and_forget ProxyGroupDeleteProxyButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                               winrt::Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        fire_and_forget LoadProxiesForProxyGroup(ProxyGroupModel const &model);

        com_ptr<AssetModel> m_model = make_self<AssetModel>();
        bool isDialogsShown{false};
        bool isDetailedViewShown{false};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage, implementation::LibraryPage>
    {
    };
}
