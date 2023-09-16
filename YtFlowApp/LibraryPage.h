#pragma once

#include "LibraryPage.g.h"

#include "AssetModel.h"
#include "CoreSubscription.h"

namespace winrt::YtFlowApp::implementation
{
    struct SubscriptionDownloadDecodeResult
    {
        std::vector<uint8_t> proxies;
        char const *format{nullptr};
        DecodedSubscriptionUserInfo userinfo;
        char const *expiresAt{nullptr};
    };

    struct LibraryPage : LibraryPageT<LibraryPage>
    {
        LibraryPage();

        YtFlowApp::AssetModel Model() const;
        fire_and_forget LoadModel();
        void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);
        void Page_Loaded(Windows::Foundation::IInspectable const &sender,
                         Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget ProxyGroupItemDelete_Click(Windows::Foundation::IInspectable const &sender,
                                                   Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupItemRename_Click(Windows::Foundation::IInspectable const &sender,
                                        Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget RenameProxyGroupItem(ProxyGroupModel const &item);
        fire_and_forget CreateProxyGroupButton_Click(Windows::Foundation::IInspectable const &sender,
                                                     Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget CreateSubscriptionButton_Click(Windows::Foundation::IInspectable const &sender,
                                                       Windows::UI::Xaml::RoutedEventArgs const &e);
        void SyncSubscriptionButton_Click(Windows::Foundation::IInspectable const &sender,
                                          Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupItem_Click(Windows::Foundation::IInspectable const &sender,
                                  Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupProxyList_SelectionChanged(
            Windows::Foundation::IInspectable const &sender,
            Windows::UI::Xaml::Controls::SelectionChangedEventArgs const &e);
        fire_and_forget ProxyGroupDeleteProxyButton_Click(Windows::Foundation::IInspectable const &sender,
                                                          Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget ProxyGroupAddProxyButton_Click(Windows::Foundation::IInspectable const &sender,
                                                       Windows::UI::Xaml::RoutedEventArgs const &e);
        void ProxyGroupShareProxyButton_Click(Windows::Foundation::IInspectable const &sender,
                                              Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        static Windows::Web::Http::HttpClient GetHttpClientForSubscription();
        static Windows::Foundation::IAsyncAction DownloadSubscriptionProxies(
            Windows::Web::Http::HttpClient client, Windows::Foundation::Uri uri, char const *format,
            std::shared_ptr<SubscriptionDownloadDecodeResult> result);
        fire_and_forget LoadProxiesForProxyGroup(ProxyGroupModel const &model);
        fire_and_forget UpdateSubscription(std::optional<uint32_t> id);

        com_ptr<AssetModel> m_model = make_self<AssetModel>();
        bool isDialogsShown{false};
        bool isDetailedViewShown{false};
        rxcpp::subjects::subject<int32_t> ProxySubscriptionUpdatesRunning$;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage, implementation::LibraryPage>
    {
    };
}
