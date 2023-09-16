#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace concurrency;

#include "ConnectionState.h"
#include "CoreFfi.h"
#include "EditProfilePage.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;

namespace winrt::YtFlowApp::implementation
{
    HomePage::HomePage()
    {
        InitializeComponent();
    }

    void HomePage::OnNavigatedFrom(Navigation::NavigationEventArgs const & /* args */)
    {
        m_connStatusChangeSubscription$.unsubscribe();
        m_refreshPluginStatus$.unsubscribe();
        PluginWidgetPanel().Children().Clear();
        ConnectedViewSidePanel().Children().Clear();
        m_widgets.clear();
    }

    IAsyncAction HomePage::EnsureDatabase()
    {
        if (FfiDbInstance != nullptr)
        {
            co_return;
        }

        const auto localFolder = appData.LocalFolder();
        const auto dbFolder =
            co_await localFolder.CreateFolderAsync(L"db", Windows::Storage::CreationCollisionOption::OpenIfExists);
        hstring const dbPath = dbFolder.Path() + L"\\main.db";
        appData.LocalSettings().Values().Insert(L"YTFLOW_DB_PATH", box_value(dbPath));

        FfiDbInstance = std::move(FfiDb::Open(dbPath));
    }

    HomePage::RequestSender HomePage::MakeRequestSender(uint32_t pluginId)
    {
        auto weak = get_weak();
        return [weak = std::move(weak), pluginId](std::string_view func, std::vector<uint8_t> param) {
            return [](auto weak, auto id, auto func, auto param) -> task<std::vector<uint8_t>> {
                auto self{weak.get()};
                // TODO: self is nullptr
                auto const ret{co_await self->m_rpc.load()->SendRequestToPlugin(id, func, std::move(param))};
                self->m_triggerInfoUpdate$.get_subscriber().on_next(true);
                co_return ret;
            }(weak, pluginId, func, param);
        };
    }

    Collections::IObservableVector<YtFlowApp::ProfileModel> HomePage::Profiles() const
    {
        return m_profiles;
    }

    void HomePage::OnConnectRequested(Windows::Foundation::IInspectable const & /* sender */,
                                      HomeProfileControl const &control)
    {
        try
        {
            if (m_vpnTask != nullptr)
            {
                m_vpnTask.Cancel();
            }
            m_vpnTask = connectToProfile(control.Profile().Id());
        }
        catch (...)
        {
            NotifyException(L"Connect request");
        }
    }
    void HomePage::OnEditRequested(Windows::Foundation::IInspectable const & /* sender */,
                                   HomeProfileControl const &control)
    {
        Frame().Navigate(xaml_typename<YtFlowApp::EditProfilePage>(), control.Profile(),
                         Media::Animation::DrillInNavigationTransitionInfo{});
    }
    fire_and_forget HomePage::OnDeleteRequested(Windows::Foundation::IInspectable const & /* sender */,
                                                HomeProfileControl const &control)
    {
        try
        {
            static bool deleting = false;
            if (deleting)
            {
                co_return;
            }
            deleting = true;
            auto const lifetime{get_strong()};
            auto const profile = control.Profile();
            ConfirmProfileDeleteDialog().Content(profile);
            auto const ret{co_await ConfirmProfileDeleteDialog().ShowAsync()};
            if (ret != ContentDialogResult::Primary)
            {
                deleting = false;
                co_return;
            }
            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            conn.DeleteProfile(profile.Id());
            co_await resume_foreground(Dispatcher());
            deleting = false;
            ConfirmProfileDeleteDialog().Content(nullptr);
            uint32_t index;
            if (!m_profiles.IndexOf(profile, index))
            {
                co_return;
            }
            m_profiles.RemoveAt(index);
        }
        catch (...)
        {
            NotifyException(L"Deleting profile");
        }
    }
    IAsyncAction HomePage::connectToProfile(uint32_t id)
    {
        auto const localSettings = appData.LocalSettings().Values();
        localSettings.Insert(L"YTFLOW_PROFILE_ID", box_value(id));
        auto connectTask{ConnectionState::Instance->Connect()};
        auto const cancelToken{co_await get_cancellation_token()};
        cancelToken.callback([=]() {
            connectTask.Cancel();
            localSettings.TryRemove(YTFLOW_CORE_ERROR_LOAD);
        });
        auto ret = co_await connectTask;
        if (ret == VpnManagementErrorStatus::Other)
        {
            auto coreError = localSettings.TryLookup(YTFLOW_CORE_ERROR_LOAD).try_as<hstring>();
            if (!coreError.has_value())
            {
                NotifyUser(hstring{std::format(L"Failed to connect VPN. Please connect YtFlow Auto using system "
                                               L"settings for detailed error messages..")},
                           L"VPN Error");
            }
        }
        else if (ret != VpnManagementErrorStatus::Ok)
        {
            NotifyUser(hstring{std::format(L"Failed to connect VPN: error code {}", static_cast<int32_t>(ret))},
                       L"VPN Error");
        }
    }

    void HomePage::ConnectCancelButton_Click(IInspectable const & /* sender */,
                                             RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
            m_vpnTask = nullptr;
        }
    }

    void HomePage::DisconnectButton_Click(IInspectable const & /* sender */,
                                          RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
        }
        m_vpnTask = []() -> IAsyncAction { co_await ConnectionState::Instance->Disconnect(); }();
    }
    void HomePage::CreateProfileButton_Click(IInspectable const & /* sender */,
                                             RoutedEventArgs const & /* e */)
    {
        Frame().Navigate(xaml_typename<NewProfilePage>());
    }
}
