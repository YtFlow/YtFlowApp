#include "pch.h"
#include "LibraryPage.h"
#if __has_include("LibraryPage.g.cpp")
#include "LibraryPage.g.cpp"
#endif

#include "CoreFfi.h"
#include "ProxyGroupModel.h"
#include "ProxyModel.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    using Windows::Foundation::IInspectable;

    LibraryPage::LibraryPage()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

        LoadModel();
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
            proxyGroupModels.reserve(proxyGroups.size());
            std::transform(proxyGroups.begin(), proxyGroups.end(), std::back_inserter(proxyGroupModels),
                           [](auto const &group) { return make<ProxyGroupModel>(group); });
            co_await resume_foreground(Dispatcher());
            m_model->ProxyGroups(single_threaded_observable_vector(std::move(proxyGroupModels)));
            ;
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

    fire_and_forget LibraryPage::ProxyGroupItemDelete_Click(IInspectable const &sender, RoutedEventArgs const &e)
    {
        try
        {
            auto const item = sender.as<FrameworkElement>().DataContext().try_as<ProxyGroupModel>();
            if (item == nullptr)
            {
                co_return;
            }

            if (std::exchange(isProxyGroupDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();
            ProxyGroupDeleteDialog().Content(*item);
            auto const dialogResult = co_await ProxyGroupDeleteDialog().ShowAsync();
            lifetime->isProxyGroupDialogsShown = false;
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
            if (std::exchange(isProxyGroupDialogsShown, true))
            {
                co_return;
            }
            auto const lifetime = get_strong();
            auto item = model.as<ProxyGroupModel>();

            ProxyGroupRenameDialogText().Text(item->Name());
            auto const dialogResult = co_await ProxyGroupRenameDialog().ShowAsync();
            lifetime->isProxyGroupDialogsShown = false;
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

            auto const newGroupName = std::string("New Group ") + std::to_string(m_model->ProxyGroups().Size());
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

}
