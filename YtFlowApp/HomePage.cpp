#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace concurrency;

#include "ConnectionState.h"
#include "CoreFfi.h"
#include "CoreRpc.h"
#include "EditProfilePage.h"
#include "FirstTimePage.h"
#include "NewProfilePage.h"
#include "ProfileModel.h"
#include "Rx.h"
#include "UI.h"
#include "WinrtScheduler.h"

#include "DynOutboundHomeWidget.h"
#include "NetifHomeWidget.h"
#include "SwitchHomeWidget.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace std::literals::chrono_literals;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    HomePage::HomePage()
    {
        InitializeComponent();
    }

    fire_and_forget HomePage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const & /* args */)
    {
        try
        {
            static IAsyncAction loadDbTask = {nullptr};
            if (loadDbTask == nullptr)
            {
                loadDbTask = std::move(EnsureDatabase());
            }
            const auto lifetime{get_strong()};

            if (!ConnectionState::Instance.has_value())
            {
                // Ensure profile exists
                const auto &profile = co_await ConnectionState::GetInstalledVpnProfile();
                if (profile == nullptr)
                {
                    co_await 400ms;
                    co_await resume_foreground(Dispatcher());
                    Frame().Navigate(xaml_typename<YtFlowApp::FirstTimePage>());
                    co_return;
                }
                else
                {
                    ConnectionState::Instance.emplace(profile);
                }
            }
            if (loadDbTask.Status() != AsyncStatus::Completed)
            {
                co_await loadDbTask;
            }
            auto conn = FfiDbInstance.Connect();
            auto profiles = conn.GetProfiles();
            std::vector<YtFlowApp::ProfileModel> profileModels;
            profileModels.reserve(profiles.size());
            std::transform(profiles.begin(), profiles.end(), std::back_inserter(profileModels),
                           [](auto const &p) { return winrt::make<YtFlowApp::implementation::ProfileModel>(p); });
            co_await resume_foreground(Dispatcher());
            if (profileModels.empty() &&
                Frame().CurrentSourcePageType().Name == xaml_typename<YtFlowApp::HomePage>().Name)
            {
                Frame().Navigate(xaml_typename<YtFlowApp::NewProfilePage>(), box_value(true));
                co_return;
            }
            m_profiles = winrt::single_threaded_observable_vector(std::move(profileModels));
            auto mainContainer = MainContainer();
            m_connStatusChangeSubscription$ =
                ConnectionState::Instance->ConnectStatusChange$.observe_on(ObserveOnDispatcher())
                    .subscribe(
                        [=](auto state) {
                            try
                            {
                                auto const localSettings = appData.LocalSettings().Values();
                                auto const coreError =
                                    localSettings.TryLookup(YTFLOW_CORE_ERROR_LOAD).try_as<hstring>();
                                if (coreError.has_value())
                                {
                                    localSettings.Remove(YTFLOW_CORE_ERROR_LOAD);
                                    NotifyUser(hstring{std::format(L"YtFlow Core failed to start. {}", *coreError)},
                                               L"Core Error");
                                }
                                switch (state)
                                {
                                case VpnManagementConnectionStatus::Disconnected:
                                    m_refreshPluginStatus$.unsubscribe();
                                    VisualStateManager::GoToState(*lifetime, L"Disconnected", true);
                                    break;
                                case VpnManagementConnectionStatus::Disconnecting:
                                    VisualStateManager::GoToState(*lifetime, L"Disconnecting", true);
                                    break;
                                case VpnManagementConnectionStatus::Connected:
                                    lifetime->SubscribeRefreshPluginStatus();
                                    lifetime->CurrentProfileNameRun().Text([&] {
                                        if (auto id{localSettings.TryLookup(L"YTFLOW_PROFILE_ID").try_as<uint32_t>()};
                                            id.has_value())
                                        {
                                            for (auto const p : lifetime->m_profiles)
                                            {
                                                if (p.Id() == *id)
                                                {
                                                    return p.Name();
                                                }
                                            }
                                        }
                                        return hstring{};
                                    }());
                                    VisualStateManager::GoToState(*lifetime, L"Connected", true);
                                    break;
                                case VpnManagementConnectionStatus::Connecting:
                                    VisualStateManager::GoToState(*lifetime, L"Connecting", true);
                                    break;
                                }
                            }
                            catch (...)
                            {
                                NotifyException(L"HomePage ConnectStatusChange subscribe");
                            }
                        },
                        [](auto ex) {
                            try
                            {
                                if (ex)
                                {
                                    std::rethrow_exception(ex);
                                }
                            }
                            catch (...)
                            {
                                NotifyException(L"HomePage ConnectStatusChange subscribe error");
                            }
                        });
            Bindings->Update();
        }
        catch (...)
        {
            NotifyException(L"HomePage NavigatedTo");
        }
    }

    void HomePage::SubscribeRefreshPluginStatus()
    {
        m_refreshPluginStatus$.unsubscribe();
        PluginWidgetPanel().Children().Clear();
        m_widgets.clear();
        m_refreshPluginStatus$ =
            rxcpp::observable<>::create<CoreRpc>([weak{get_weak()}](rxcpp::subscriber<CoreRpc> s) {
                [](auto s, auto weak) -> fire_and_forget {
                    try
                    {
                        auto rpc = co_await CoreRpc::Connect();
                        if (auto const self{weak.get()}; self)
                        {
                            self->m_rpc = std::make_shared<CoreRpc>(rpc);
                        }
                        s.add([=]() { rpc.m_socket.Close(); });
                        s.on_next(std::move(rpc));
                    }
                    catch (...)
                    {
                        s.on_error(std::current_exception());
                    }
                }(std::move(s), weak);
            })
                .flat_map([weak{get_weak()}](auto rpc) {
                    auto const focus${ObserveApplicationLeavingBackground()};
                    auto const unfocus${ObserveApplicationEnteredBackground()};
                    auto hashcodes{std::make_shared<std::map<uint32_t, uint32_t>>()};
                    return focus$.start_with(true).flat_map([=](auto) {
                        auto const self{weak.get()};
                        if (!self)
                        {
                            throw std::runtime_error("Cannot subscribe when HomePage disposed");
                        }
                        return rxcpp::observable<>::interval(1s)
                            .map([](auto) { return true; })
                            .merge(self->m_triggerInfoUpdate$.get_observable())
                            .concat_map(
                                [=](auto) { return Rx::observe_awaitable(rpc.CollectAllPluginInfo(hashcodes)); })
                            .map([=](auto const &&info) {
                                for (auto const &p : info)
                                {
                                    (*hashcodes)[p.id] = p.hashcode;
                                }
                                return std ::move(info);
                            })
                            .tap([](auto const &) {},
                                 [](auto ex) {
                                     try
                                     {
                                         std::rethrow_exception(ex);
                                     }
                                     catch (RpcException const &e)
                                     {
                                         NotifyUser(to_hstring(e.msg), L"RPC Error");
                                     }
                                 })
                            .subscribe_on(ObserveOnWinrtThreadPool())
                            .take_until(unfocus$);
                    });
                })
                .on_error_resume_next([](auto) {
                    return rxcpp::observable<>::timer(3s).flat_map([](auto) {
                        return rxcpp::sources::error<std::vector<RpcPluginInfo>>("Retry connecting Core RPC");
                    });
                })
                .retry()
                .subscribe_on(ObserveOnWinrtThreadPool())
                .observe_on(ObserveOnDispatcher())
                .subscribe(
                    [weak{get_weak()}](auto info) {
                        auto const self{weak.get()};
                        if (!self)
                        {
                            return;
                        }
                        for (auto const &plugin : info)
                        {
                            try
                            {
                                // Append/update only. No deletion required at this moment.
                                auto it = self->m_widgets.find(plugin.id);
                                if (it == self->m_widgets.end())
                                {
                                    auto handle{self->CreateWidgetHandle(plugin)};
                                    if (!handle.has_value())
                                    {
                                        continue;
                                    }
                                    it = self->m_widgets.emplace(std::make_pair(plugin.id, std::move(*handle))).first;
                                }
                                *it->second.info = plugin.info;
                                if (auto const widget{it->second.widget.get()})
                                {
                                    widget.UpdateInfo();
                                }
                            }
                            catch (...)
                            {
                                NotifyException(hstring(L"Plugin status RPC subscribe: ") + to_hstring(plugin.name));
                            }
                        }
                    },
                    [](auto ex) {
                        try
                        {
                            if (ex)
                            {
                                std::rethrow_exception(ex);
                            }
                        }
                        catch (...)
                        {
                            NotifyException(L"Plugin status RPC subscribe error");
                        }
                    });
    }

    void HomePage::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const & /* args */)
    {
        m_connStatusChangeSubscription$.unsubscribe();
        m_refreshPluginStatus$.unsubscribe();
        PluginWidgetPanel().Children().Clear();
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

    std::optional<HomePage::WidgetHandle> HomePage::CreateWidgetHandle(RpcPluginInfo const &info)
    {
        HomePage::WidgetHandle handle;
        handle.info = std::make_shared<std::vector<uint8_t>>();
        if (info.plugin == "netif")
        {
            auto widget{winrt::make<NetifHomeWidget>(to_hstring(info.name), handle.info)};
            handle.widget = widget;
            PluginWidgetPanel().Children().Append(std::move(widget));
        }
        else if (info.plugin == "switch")
        {
            auto widget = winrt::make<SwitchHomeWidget>(to_hstring(info.name), handle.info, MakeRequestSender(info.id));
            handle.widget = widget;
            PluginWidgetPanel().Children().Append(std::move(widget));
        }
        else if (info.plugin == "dyn-outbound")
        {
            auto widget =
                winrt::make<DynOutboundHomeWidget>(to_hstring(info.name), handle.info, MakeRequestSender(info.id));
            handle.widget = widget;
            PluginWidgetPanel().Children().Append(std::move(widget));
        }
        else
        {
            return {std::nullopt};
        }
        return handle;
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

    Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProfileModel> HomePage::Profiles() const
    {
        return m_profiles;
    }

    void HomePage::OnConnectRequested(Windows::Foundation::IInspectable const & /* sender */,
                                      winrt::YtFlowApp::HomeProfileControl const &control)
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
                                   winrt::YtFlowApp::HomeProfileControl const &control)
    {
        Frame().Navigate(xaml_typename<YtFlowApp::EditProfilePage>(), control.Profile(),
                         Media::Animation::DrillInNavigationTransitionInfo{});
    }
    fire_and_forget HomePage::OnDeleteRequested(Windows::Foundation::IInspectable const & /* sender */,
                                                winrt::YtFlowApp::HomeProfileControl const &control)
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
        auto const cancelToken{co_await winrt::get_cancellation_token()};
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
                                             Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
            m_vpnTask = nullptr;
        }
    }

    void HomePage::DisconnectButton_Click(IInspectable const & /* sender */,
                                          winrt::Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
        }
        m_vpnTask = []() -> IAsyncAction { co_await ConnectionState::Instance->Disconnect(); }();
    }
    void HomePage::CreateProfileButton_Click(IInspectable const & /* sender */,
                                             winrt::Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        Frame().Navigate(xaml_typename<YtFlowApp::NewProfilePage>());
    }
}
