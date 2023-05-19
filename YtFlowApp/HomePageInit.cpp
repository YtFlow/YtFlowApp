#include "pch.h"
#include "HomePage.h"

#include "ConnectionState.h"
#include "CoreFfi.h"
#include "ProfileModel.h"
#include "UI.h"
#include "WinrtScheduler.h"

namespace winrt::YtFlowApp::implementation
{
    using namespace std::literals::chrono_literals;
    using namespace Windows::UI::Xaml;

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
                                    PluginWidgetPanel().Children().Clear();
                                    ConnectedViewSidePanel().Children().Clear();
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

}
