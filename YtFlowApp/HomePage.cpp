#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

#include "ConnectionState.h"
#include "CoreFfi.h"
#include "EditProfilePage.h"
#include "FirstTimePage.h"
#include "NewProfilePage.h"
#include "ProfileModel.h"
#include "RxDispatcherScheduler.h"
#include "UI.h"
#include <format>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace std::literals::chrono_literals;

namespace winrt::YtFlowApp::implementation
{
    HomePage::HomePage()
    {
        InitializeComponent();
    }

    fire_and_forget HomePage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const & /* args */)
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
        m_profiles = winrt::single_threaded_observable_vector(std::move(profileModels));
        auto mainContainer = MainContainer();
        m_connStatusChangeSubscription =
            ConnectionState::Instance->ConnectStatusChange$.observe_on(ObserveOnDispatcher())
                .subscribe([=](auto state) {
                    auto localSettings = appData.LocalSettings().Values();
                    auto coreError =
                        appData.LocalSettings().Values().TryLookup(YTFLOW_CORE_ERROR_LOAD).try_as<hstring>();
                    if (coreError.has_value())
                    {
                        localSettings.Remove(YTFLOW_CORE_ERROR_LOAD);
                        NotifyUser(hstring{std::format(L"YtFlow Core failed to start. {}", *coreError)}, L"Core Error");
                    }
                    switch (state)
                    {
                    case VpnManagementConnectionStatus::Disconnected:
                        VisualStateManager::GoToState(*lifetime, L"Disconnected", true);
                        break;
                    case VpnManagementConnectionStatus::Disconnecting:
                        VisualStateManager::GoToState(*lifetime, L"Disconnecting", true);
                        break;
                    case VpnManagementConnectionStatus::Connected:
                        VisualStateManager::GoToState(*lifetime, L"Connected", true);
                        break;
                    case VpnManagementConnectionStatus::Connecting:
                        VisualStateManager::GoToState(*lifetime, L"Connecting", true);
                        break;
                    }
                });
        Bindings->Update();
    }

    void HomePage::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const & /* args */)
    {
        m_connStatusChangeSubscription.unsubscribe();
    }

    IAsyncAction HomePage::EnsureDatabase()
    {
        if (FfiDbInstance != nullptr)
        {
            co_return;
        }

        const auto &localFolder = appData.LocalFolder();
        const auto &dbFolder =
            co_await localFolder.CreateFolderAsync(L"db", Windows::Storage::CreationCollisionOption::OpenIfExists);
        hstring dbPath = dbFolder.Path() + L"\\main.db";
        appData.LocalSettings().Values().Insert(L"YTFLOW_DB_PATH", box_value(dbPath));

        FfiDbInstance = std::move(FfiDb::Open(dbPath));
    }

    Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProfileModel> HomePage::Profiles() const
    {
        return m_profiles;
    }

    void HomePage::OnConnectRequested(Windows::Foundation::IInspectable const & /* sender */,
                                      winrt::YtFlowApp::HomeProfileControl const &control)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
        }
        m_vpnTask = connectToProfile(control.Profile().Id());
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
        static bool deleting = false;
        if (deleting)
        {
            co_return;
        }
        deleting = true;
        const auto lifetime{get_strong()};
        auto const profile = control.Profile();
        ConfirmProfileDeleteDialog().Content(profile);
        const auto ret{co_await ConfirmProfileDeleteDialog().ShowAsync()};
        if (ret != ContentDialogResult::Primary)
        {
            deleting = false;
            co_return;
        }
        co_await resume_background();
        const auto conn{FfiDbInstance.Connect()};
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
    IAsyncAction HomePage::connectToProfile(uint32_t id)
    {
        auto localSettings = appData.LocalSettings().Values();
        localSettings.Insert(L"YTFLOW_PROFILE_ID", box_value(id));
        auto connectTask{ConnectionState::Instance->Connect()};
        auto cancelToken{co_await winrt::get_cancellation_token()};
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
        m_vpnTask = ([]() -> IAsyncAction { co_await ConnectionState::Instance->Disconnect(); })();
    }
    void HomePage::CreateProfileButton_Click(IInspectable const & /* sender */,
                                             winrt::Windows::UI::Xaml::RoutedEventArgs const & /* e */)
    {
        Frame().Navigate(xaml_typename<YtFlowApp::NewProfilePage>());
    }
}
