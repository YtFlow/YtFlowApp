﻿#include "pch.h"
#include "LibraryPage.h"
#if __has_include("LibraryPage.g.cpp")
#include "LibraryPage.g.cpp"
#endif

#include "winrt\Windows.Web.Http.Headers.h"
#include <ranges>

#include "CoreFfi.h"
#include "CoreProxy.h"
#include "CoreSubscription.h"
#include "ProxyGroupModel.h"
#include "ProxyModel.h"
#include "UI.h"

#include "EditProxyPageParam.h"

using namespace std::string_view_literals;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;

namespace winrt::YtFlowApp::implementation
{
    using Windows::Foundation::IInspectable;

    LibraryPage::LibraryPage()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

        LoadModel();
    }

    Windows::UI::Xaml::DependencyProperty LibraryPage::IsProxyGroupProxySingleSelectedProperty()
    {
        return m_isProxyGroupProxySingleSelectedProperty;
    }

    fire_and_forget LibraryPage::LoadModel()
    {
        try
        {
            auto lifetime{get_strong()};
            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            auto const proxyGroups = conn.GetProxyGroups();
            std::vector<YtFlowApp::ProxyGroupModel> proxyGroupModels;
            std::vector<std::pair<ProxyGroupModel *, FfiProxyGroupSubscription>> subscriptionInfoToAttach;
            proxyGroupModels.reserve(proxyGroups.size());
            subscriptionInfoToAttach.reserve(proxyGroups.size());
            std::transform(proxyGroups.begin(), proxyGroups.end(), std::back_inserter(proxyGroupModels),
                           [&conn, &subscriptionInfoToAttach](auto const &group) {
                               auto ret = make<ProxyGroupModel>(group);
                               if (group.type == "subscription")
                               {
                                   subscriptionInfoToAttach.emplace_back(
                                       std::make_pair(get_self<ProxyGroupModel>(ret),
                                                      conn.GetProxySubscriptionByProxyGroup(group.id)));
                               }
                               return ret;
                           });
            co_await resume_foreground(Dispatcher());
            for (auto &&[model, subscriptionInfo] : subscriptionInfoToAttach)
            {
                model->AttachSubscriptionInfo(subscriptionInfo);
            }
            m_model->ProxyGroups(single_threaded_observable_vector(std::move(proxyGroupModels)));
        }
        catch (...)
        {
            NotifyException(L"Loading Library page");
        }
    }

    YtFlowApp::AssetModel LibraryPage::Model() const
    {
        return *m_model;
    }

    void LibraryPage::OnNavigatingFrom(Navigation::NavigatingCancelEventArgs const &args)
    {
        if (args.Cancel())
        {
            return;
        }
        if (args.NavigationMode() == Navigation::NavigationMode::Back && isDetailedViewShown)
        {
            VisualStateManager::GoToState(*this, L"DisplayAssetView", true);
            args.Cancel(true);
            isDetailedViewShown = false;
        }
    }

    void LibraryPage::Page_Loaded(IInspectable const &, RoutedEventArgs const &)
    {
        ProxySubscriptionUpdatesRunning$.get_observable()
            .scan(std::make_pair(0, 0),
                  [](auto prev, int curr) { return std::make_pair(prev.second, prev.second + curr); })
            .filter([](auto change) { return change.first == 0 || change.second == 0; })
            .subscribe([weak{get_weak()}](auto change) {
                if (auto const self = weak.get())
                {
                    if (change.first == 0)
                    {
                        self->SyncSubscriptionButtonRunStoryboard().Begin();
                    }
                    else
                    {
                        self->SyncSubscriptionButtonRunStoryboard().Stop();
                    }
                }
            });
    }

    fire_and_forget LibraryPage::ProxyGroupItemDelete_Click(IInspectable const &sender, RoutedEventArgs const &e)
    {
        try
        {
            auto const item = sender.as<FrameworkElement>().DataContext().try_as<ProxyGroupModel>();
            if (item == nullptr)
            {
                co_return;
            }

            if (std::exchange(isDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();
            ProxyGroupDeleteDialog().Content(*item);
            auto const dialogResult = co_await ProxyGroupDeleteDialog().ShowAsync();
            lifetime->isDialogsShown = false;
            if (dialogResult != ContentDialogResult::Primary)
            {
                co_return;
            }

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            conn.DeleteProxyGroup(item->Id());
            co_await resume_foreground(lifetime->Dispatcher());

            auto const collection = lifetime->m_model->ProxyGroups();
            uint32_t index{0};
            if (!collection.IndexOf(*item, index))
            {
                co_return;
            }
            collection.RemoveAt(index);
        }
        catch (...)
        {
            NotifyException(L"Deleting");
        }
    }
    void LibraryPage::ProxyGroupItemRename_Click(IInspectable const &sender, RoutedEventArgs const &)
    {
        auto const item = sender.as<FrameworkElement>().DataContext().try_as<ProxyGroupModel>();
        if (item == nullptr)
        {
            return;
        }
        RenameProxyGroupItem(*item);
    }
    fire_and_forget LibraryPage::RenameProxyGroupItem(YtFlowApp::ProxyGroupModel const &model)
    {
        try
        {
            if (std::exchange(isDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();
            auto item = model.as<ProxyGroupModel>();

            ProxyGroupRenameDialogText().Text(item->Name());
            auto const dialogResult = co_await ProxyGroupRenameDialog().ShowAsync();
            lifetime->isDialogsShown = false;
            if (dialogResult != ContentDialogResult::Primary)
            {
                co_return;
            }
            auto const newNameW = ProxyGroupRenameDialogText().Text();
            auto const newName = to_string(newNameW);

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            conn.RenameProxyGroup(item->Id(), newName.c_str());
            co_await resume_foreground(lifetime->Dispatcher());

            item->Name(newNameW);
        }
        catch (...)
        {
            NotifyException(L"Renaming");
        }
    }

    fire_and_forget LibraryPage::CreateProxyGroupButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        try
        {
            auto const lifetime = get_strong();

            auto const newGroupName = std::string("New Group ") + std::to_string(m_model->ProxyGroups().Size() + 1);
            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            auto const newGroupId = conn.CreateProxyGroup(newGroupName.c_str(), "manual");
            auto const newGroupModel = make<ProxyGroupModel>(conn.GetProxyGroupById(newGroupId));
            co_await resume_foreground(lifetime->Dispatcher());

            m_model->ProxyGroups().Append(newGroupModel);
            RenameProxyGroupItem(newGroupModel);
        }
        catch (...)
        {
            NotifyException(L"Creating");
        }
    }

    Windows::Web::Http::HttpClient LibraryPage::GetHttpClientForSubscription()
    {
        static Windows::Web::Http::HttpClient client{nullptr};
        if (client == nullptr)
        {
            client = Windows::Web::Http::HttpClient();
            client.DefaultRequestHeaders().UserAgent().Clear();
            client.DefaultRequestHeaders().UserAgent().ParseAdd(L"YtFlowApp/0.0 SubscriptionUpdater/0.0");
        }
        return client;
    }
    IAsyncAction LibraryPage::DownloadSubscriptionProxies(Windows::Web::Http::HttpClient client, Uri uri,
                                                          char const *formatInput,
                                                          std::shared_ptr<SubscriptionDownloadDecodeResult> result)
    {
        auto format = formatInput;

        auto const res = (co_await client.GetAsync(uri)).EnsureSuccessStatusCode();
        auto const userinfoHeader = res.Headers().TryLookup(L"subscription-userinfo");
        DecodedSubscriptionUserInfo userinfo{};
        if (userinfoHeader.has_value())
        {
            userinfo = DecodeSubscriptionUserInfoFromResponseHeaderValue(to_string(*userinfoHeader));
        }

        auto const resStr = to_string(co_await res.Content().ReadAsStringAsync());
        auto proxies = DecodeSubscriptionProxies(resStr, format);
        if (!proxies.has_value() || format == nullptr)
        {
            throw hresult_invalid_argument(L"The subscription data contains no valid proxy.");
        }
        *result = SubscriptionDownloadDecodeResult{.proxies = std::move(proxies).value(),
                                                   .format = format,
                                                   .userinfo = std::move(userinfo),
                                                   .expiresAt = nullptr};
        if (result->userinfo.expires_at.has_value())
        {
            result->expiresAt = result->userinfo.expires_at->c_str();
        }
    }
    fire_and_forget LibraryPage::CreateSubscriptionButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        try
        {
            auto const lifetime = get_strong();
            ProxyGroupAddSubscriptionError().Text(L"");
            ProxyGroupAddSubscriptionError().Visibility(Visibility::Collapsed);
            auto const client = GetHttpClientForSubscription();
            while (co_await ProxyGroupAddSubscriptionDialog().ShowAsync() == ContentDialogResult::Primary)
            {
                auto const url = ProxyGroupAddSubscriptionUrlText().Text();
                std::optional<hstring> errMsg = std::nullopt;
                lifetime->ProxySubscriptionUpdatesRunning$.get_subscriber().on_next(1);

                try
                {
                    Uri const uri{url};
                    auto const res = std::make_shared<SubscriptionDownloadDecodeResult>();
                    co_await DownloadSubscriptionProxies(client, uri, nullptr, res);

                    co_await resume_background();
                    auto conn = FfiDbInstance.Connect();
                    auto const newGroupId = conn.CreateProxySubscriptionGroup(to_string(uri.Host()).c_str(),
                                                                              res->format, to_string(url).c_str());
                    conn.BatchUpdateProxyInGroup(newGroupId, res->proxies.data(), res->proxies.size());
                    conn.UpdateProxySubscriptionRetrievedByProxyGroup(newGroupId, res->userinfo.upload_bytes_used,
                                                                      res->userinfo.download_bytes_used,
                                                                      res->userinfo.bytes_total, res->expiresAt);
                    auto const newGroupModel = make<ProxyGroupModel>(conn.GetProxyGroupById(newGroupId));
                    co_await resume_foreground(lifetime->Dispatcher());
                    get_self<ProxyGroupModel>(newGroupModel)
                        ->AttachSubscriptionInfo(conn.GetProxySubscriptionByProxyGroup(newGroupId));
                    m_model->ProxyGroups().Append(newGroupModel);
                }
                catch (hresult_error const &hr)
                {
                    errMsg = {hr.message()};
                }
                co_await resume_foreground(lifetime->Dispatcher());
                lifetime->ProxySubscriptionUpdatesRunning$.get_subscriber().on_next(-1);
                if (errMsg.has_value())
                {
                    ProxyGroupAddSubscriptionError().Text(*errMsg);
                    ProxyGroupAddSubscriptionError().Visibility(Visibility::Visible);
                }
                else
                {
                    break;
                }
            }
        }
        catch (...)
        {
            NotifyException(L"Import Subscription");
        }
    }

    void LibraryPage::SyncSubscriptionButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        UpdateSubscription(std::nullopt);
    }

    fire_and_forget LibraryPage::UpdateSubscription(std::optional<uint32_t> id)
    {
        auto const lifetime = get_strong();
        auto proxyGroupIt =
            std::ranges::views::transform(lifetime->m_model->ProxyGroups(),
                                          [](auto const &model) { return model.as<ProxyGroupModel>(); }) |
            std::ranges::views::filter([&id](auto const &model) {
                return !model->IsManualGroup() && !model->IsUpdating && (!id.has_value() || model->Id() == *id);
            });
        std::vector const proxyGroups(proxyGroupIt.begin(), proxyGroupIt.end());
        try
        {
            for (auto &&model : proxyGroups)
            {
                model->IsUpdating = true;
            }
            lifetime->ProxySubscriptionUpdatesRunning$.get_subscriber().on_next(1);
            std::vector<std::pair<com_ptr<ProxyGroupModel>, FfiProxyGroupSubscription>> subscriptionInfoToAttach;
            subscriptionInfoToAttach.reserve(proxyGroups.size());
            auto const client = GetHttpClientForSubscription();

            co_await resume_background();

            hstring errors;
            auto res = std::make_shared<SubscriptionDownloadDecodeResult>();
            auto conn = FfiDbInstance.Connect();
            for (auto &&model : proxyGroups)
            {
                try
                {
                    auto const groupId = model->Id();
                    auto const subscription = conn.GetProxySubscriptionByProxyGroup(groupId);
                    co_await DownloadSubscriptionProxies(client, Uri{to_hstring(subscription.url)},
                                                         subscription.format.data(), res);
                    conn.BatchUpdateProxyInGroup(groupId, res->proxies.data(), res->proxies.size());
                    conn.UpdateProxySubscriptionRetrievedByProxyGroup(groupId, res->userinfo.upload_bytes_used,
                                                                      res->userinfo.download_bytes_used,
                                                                      res->userinfo.bytes_total, res->expiresAt);
                    subscriptionInfoToAttach.emplace_back(
                        std::make_pair(model, conn.GetProxySubscriptionByProxyGroup(groupId)));
                }
                catch (hresult_error const &hr)
                {
                    errors = errors + hstring{L"\r\n"} + model->Name() + L": " + hr.message();
                }
            }

            co_await resume_foreground(lifetime->Dispatcher());
            for (auto &&[model, subscriptionInfo] : subscriptionInfoToAttach)
            {
                model->AttachSubscriptionInfo(subscriptionInfo);
                if (model->Proxies() != nullptr)
                {
                    model->Proxies(nullptr);
                    LoadProxiesForProxyGroup(*model);
                }
            }
            if (!errors.empty())
            {
                NotifyUser(errors, L"Update errors");
            }
        }
        catch (...)
        {
            NotifyException(L"Updating Subscription");
        }
        co_await resume_foreground(lifetime->Dispatcher());
        for (auto &&model : proxyGroups)
        {
            model->IsUpdating = false;
        }
        lifetime->ProxySubscriptionUpdatesRunning$.get_subscriber().on_next(-1);
    }

    void LibraryPage::ProxyGroupItem_Click(IInspectable const &sender, RoutedEventArgs const &)
    {
        auto const source = sender.as<FrameworkElement>();
        auto const model = source.DataContext().try_as<ProxyGroupModel>();
        if (model == nullptr)
        {
            return;
        }
        LoadProxiesForProxyGroup(*model);
        m_model->CurrentProxyGroupModel(*model);
        VisualStateManager::GoToState(*this, L"DisplayProxyGroupView", true);
        isDetailedViewShown = true;
    }

    fire_and_forget LibraryPage::LoadProxiesForProxyGroup(YtFlowApp::ProxyGroupModel const &model)
    {
        try
        {
            auto const lifetime = get_strong();
            auto const modelLifetime = model;
            auto const proxyGroup = modelLifetime.as<ProxyGroupModel>();
            if (proxyGroup->Proxies() != nullptr)
            {
                co_return;
            }
            auto const proxyGroupId = proxyGroup->Id();

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            auto const ffiProxies = conn.GetProxiesByProxyGroup(proxyGroupId);
            std::vector<YtFlowApp::ProxyModel> proxies;
            proxies.reserve(ffiProxies.size());
            std::transform(ffiProxies.begin(), ffiProxies.end(), std::back_inserter(proxies),
                           [](auto const &ffiProxy) { return make<ProxyModel>(ffiProxy); });
            auto const collection = single_threaded_observable_vector(std::move(proxies));
            co_await resume_foreground(lifetime->Dispatcher());

            proxyGroup->Proxies(std::move(collection));
        }
        catch (...)
        {
            NotifyException(L"Loading proxies for proxy group");
        }
    }

    void LibraryPage::ProxyGroupProxyList_SelectionChanged(IInspectable const &sender,
                                                           SelectionChangedEventArgs const &)
    {
        auto const isEmpty = sender.as<ListView>().SelectedItems().Size() == 0;
        ProxyGroupShareProxyButton().IsEnabled(!isEmpty);
        auto const currentProxyGroup = m_model->CurrentProxyGroupModel();
        if (currentProxyGroup != nullptr)
        {
            ProxyGroupDeleteProxyButton().IsEnabled(currentProxyGroup.IsManualGroup() && !isEmpty);
        }
        SetValue(m_isProxyGroupProxySingleSelectedProperty, box_value(IsProxyGroupProxySingleSelected()));
    }

    fire_and_forget LibraryPage::ProxyGroupDeleteProxyButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        try
        {
            if (std::exchange(isDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();
            auto const selected = ProxyGroupProxyList().SelectedItems();
            auto const proxyGroup = m_model->CurrentProxyGroupModel();
            if (proxyGroup == nullptr)
            {
                co_return;
            }

            switch (selected.Size())
            {
            case 0:
                co_return;
            case 1: {
                auto const proxy = selected.GetAt(0).try_as<ProxyModel>();
                if (proxy == nullptr)
                {
                    co_return;
                }
                ProxyGroupDeleteProxyPlaceholder().Text(proxy->Name());
            }
            break;
            default:
                ProxyGroupDeleteProxyPlaceholder().Text(to_hstring(selected.Size()) + L" proxies");
                break;
            }
            auto const dialogResult = co_await ProxyGroupProxyDeleteDialog().ShowAsync();
            lifetime->isDialogsShown = false;
            if (dialogResult != ContentDialogResult::Primary)
            {
                co_return;
            }
            std::set<uint32_t> proxyIds;
            for (auto const selectedObj : selected)
            {
                auto const proxy = selectedObj.try_as<ProxyModel>();
                if (proxy == nullptr)
                {
                    continue;
                }
                proxyIds.insert(proxy->Id());
            }

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            for (auto const id : proxyIds)
            {
                conn.DeleteProxy(id);
            }
            co_await resume_foreground(lifetime->Dispatcher());

            auto const existingProxies = get_self<ProxyGroupModel>(proxyGroup)->Proxies();
            for (uint32_t i = 0; i < existingProxies.Size();)
            {
                if (proxyIds.contains(get_self<ProxyModel>(existingProxies.GetAt(i))->Id()))
                {
                    existingProxies.RemoveAt(i);
                }
                else
                {
                    i++;
                }
            }
        }
        catch (...)
        {
            NotifyException(L"Deleting proxies");
        }
    }

    fire_and_forget LibraryPage::ProxyGroupAddProxyButton_Click(IInspectable const &sender, RoutedEventArgs const &e)
    {
        try
        {
            if (std::exchange(isDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();

            ProxyGroupProxyImportText().Text(L"");
            auto const dialogResult = co_await ProxyGroupProxyImportDialog().ShowAsync();
            lifetime->isDialogsShown = false;
            if (dialogResult != ContentDialogResult::Primary)
            {
                co_return;
            }
            auto const input = ProxyGroupProxyImportText().Text();
            if (input.empty())
            {
                co_return;
            }
            auto const currentGroup = m_model->CurrentProxyGroupModel();
            auto const groupId = currentGroup.Id();

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            int unrecognized = 0;
            std::vector<uint32_t> newProxyIds;
            for (auto const wline : std::ranges::views::split(input, L"\r"sv))
            {
                auto const line(to_string(std::wstring_view(wline.begin(), wline.end())));
                constexpr char const *SPACES = " \r\n\t\v\f";
                auto const lpos = line.find_first_not_of(SPACES);
                auto const rpos = line.find_last_not_of(SPACES);
                if (lpos == std::string::npos || rpos == std::string::npos)
                {
                    continue;
                }
                std::string trimmedLink(line, lpos, rpos - lpos + 1);
                auto proxy = ConvertShareLinkToDataProxy(trimmedLink);
                if (!proxy.has_value())
                {
                    unrecognized++;
                    continue;
                }

                auto const [proxyName, proxyParam] = std::move(proxy).value();
                auto const newProxyId =
                    conn.CreateProxy(groupId, proxyName.c_str(), proxyParam.data(), proxyParam.size(), 0);
                newProxyIds.push_back(newProxyId);
            }

            auto ffiProxies = conn.GetProxiesByProxyGroup(groupId);
            std::map<uint32_t, FfiDataProxy> ffiProxySet;
            std::ranges::transform(ffiProxies, std::inserter(ffiProxySet, ffiProxySet.end()), [](auto &&ffiProxy) {
                auto const id = ffiProxy.id;
                return std::make_pair(id, std::forward<FfiDataProxy>(ffiProxy));
            });
            auto newProxyModels =
                std::views::transform(newProxyIds, [&](auto id) { return make<ProxyModel>(ffiProxySet[id]); });
            co_await resume_foreground(lifetime->Dispatcher());

            for (auto &&newProxyModel : newProxyModels)
            {
                currentGroup.Proxies().Append(std::forward<YtFlowApp::ProxyModel>(newProxyModel));
            }
            if (!newProxyIds.empty() || unrecognized > 0)
            {
                hstring unrecognizedMsg;
                if (unrecognized > 0)
                {
                    unrecognizedMsg =
                        unrecognizedMsg + L" (skipped " + to_hstring(unrecognized) + L" unrecognized link)";
                }
                NotifyUser(hstring{L"Imported "} + to_hstring(newProxyIds.size()) + L" proxies." + unrecognizedMsg,
                           L"Import proxy");
            }
        }
        catch (...)
        {
            NotifyException(L"Importing proxies");
        }
    }

    void LibraryPage::ProxyGroupShareProxyButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        auto linkRange =
            std::views::transform(ProxyGroupProxyList().SelectedItems(),
                                  [](auto const &obj) { return obj.try_as<ProxyModel>(); }) |
            std::views::filter([](auto const &model) { return model != nullptr; }) |
            std::views::transform([](auto const &model) {
                auto const proxy = model->Proxy();
                std::span<uint8_t const> const proxyView(proxy.begin(), proxy.size());
                // TODO: count unrecognized
                return ConvertDataProxyToShareLink(to_string(model->Name()), proxyView);
            }) |
            std::views::filter([](auto const &link) { return link.has_value(); }) |
            std::views::transform([](auto &&link) { return std::forward<std::optional<std::string>>(link).value(); });
        bool isFirst = true;
        hstring text;
        for (auto const link : std::move(linkRange))
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                text = text + L"\r\n";
            }
            text = text + to_hstring(link);
        }
        ProxyGroupProxyExportText().Text(std::move(text));
        auto const _ = ProxyGroupProxyExportDialog().ShowAsync();
    }

    void LibraryPage::ProxyGroupEditProxyButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        if (!IsProxyGroupProxySingleSelected())
        {
            return;
        }
        auto const proxy = ProxyGroupProxyList().SelectedItem().as<ProxyModel>();
        auto const isSubscription = m_model->CurrentProxyGroupModel().as<ProxyGroupModel>()->IsSubscription();
        Frame().Navigate(xaml_typename<YtFlowApp::EditProxyPage>(), make<EditProxyPageParam>(isSubscription, proxy));
    }
    bool LibraryPage::IsProxyGroupProxySingleSelected()
    {
        auto const selected = ProxyGroupProxyList().SelectedItems();
        if (selected.Size() != 1)
        {
            return false;
        }
        auto const proxy = selected.GetAt(0).try_as<ProxyModel>();
        return proxy != nullptr;
    }
}
