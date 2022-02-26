#include "pch.h"
#include "ConnectionState.h"
#include <cwctype>
#include <winrt/Windows.Networking.Connectivity.h>
#include <ytflow_core.h>

#include <Rx.h>
#include <WinrtScheduler.h>

using namespace std::chrono_literals;

namespace winrt::YtFlowApp::implementation
{
    std::optional<ConnectionState> ConnectionState::Instance = {std::nullopt};

    hstring StatusToString(VpnManagementConnectionStatus status)
    {
        switch (status)
        {
        case VpnManagementConnectionStatus::Connected:
            return L"Connected";
        case VpnManagementConnectionStatus::Disconnected:
            return L"Disconnected";
        case VpnManagementConnectionStatus::Connecting:
            return L"Connecting";
        case VpnManagementConnectionStatus::Disconnecting:
            return L"Disconnecting";
        }
        return L"Unknown";
    }

    ConnectionState::ConnectionState(VpnPlugInProfile profile) : m_profile(profile)
    {
        auto const focus${ObserveApplicationLeavingBackground()};
        auto const unfocus${ObserveApplicationEnteredBackground()};

        ConnectStatusChange$ = ManualManagement$.get_observable()
                                   .merge(focus$.start_with(true).flat_map([=](bool) {
                                       return rxcpp::observable<>::interval(1s, rxcpp::observe_on_event_loop())
                                           .map([=](auto) {
                                               try
                                               {
                                                   return profile.ConnectionStatus();
                                               }
                                               catch (...)
                                               {
                                                   // Hack for Windows Phone
                                                   return VpnManagementConnectionStatus::Disconnected;
                                               }
                                           })
                                           .take_until(unfocus$);
                                   }))
                                   .distinct_until_changed()
                                   .subscribe_on(ObserveOnDispatcher())
                                   .replay(1)
                                   .ref_count();
    }

    IAsyncOperation<VpnPlugInProfile> ConnectionState::GetInstalledVpnProfile()
    {
        const auto &profiles = co_await Agent.GetProfilesAsync();
        for (const auto &profile : profiles)
        {
            const auto &profileName = profile.ProfileName();
            if (profileName.size() != PROFILE_NAME.size())
            {
                continue;
            }
            for (hstring::size_type i = 0; i < PROFILE_NAME.size(); i++)
            {
                auto ch = profileName[i];
                if (std::towlower(ch) != PROFILE_NAME[i])
                {
                    goto BREAK2;
                }
            }
            co_return profile.try_as<VpnPlugInProfile>();
        BREAK2:
            continue;
        }
        co_return nullptr;
    }

    IAsyncOperation<VpnManagementErrorStatus> ConnectionState::Connect()
    {
        auto const &subscriber = ManualManagement$.get_subscriber();
        subscriber.on_next(VpnManagementConnectionStatus::Connecting);
        auto ret = co_await Agent.ConnectProfileAsync(m_profile);
        if (ret == VpnManagementErrorStatus::Ok)
        {
            subscriber.on_next(VpnManagementConnectionStatus::Connected);
        }
        co_return ret;
    }
    IAsyncOperation<VpnManagementErrorStatus> ConnectionState::Disconnect()
    {
        auto const &subscriber = ManualManagement$.get_subscriber();
        subscriber.on_next(VpnManagementConnectionStatus::Disconnecting);
        auto ret = co_await Agent.DisconnectProfileAsync(m_profile);
        if (ret == VpnManagementErrorStatus::Ok)
        {
            subscriber.on_next(VpnManagementConnectionStatus::Disconnected);
        }
        co_return ret;
    }
}