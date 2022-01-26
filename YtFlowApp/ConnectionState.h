#pragma once
#include <rxcpp/rx.hpp>
#include <winrt/Windows.Networking.Vpn.h>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Vpn;

namespace winrt::YtFlowApp::implementation
{
    enum class ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting,
    };
    // Use small cases for easier comparisons.
    constexpr std::wstring_view PROFILE_NAME = L"ytflow auto";
    struct ConnectionState
    {
        ConnectionState(VpnPlugInProfile profile);
        ConnectionState(ConnectionState const &) = delete;
        ConnectionState(ConnectionState &&) = default;
        ConnectionState &operator=(ConnectionState const &) = delete;

        static IAsyncOperation<VpnPlugInProfile> GetInstalledVpnProfile();

        static std::optional<ConnectionState> Instance;
        static inline VpnManagementAgent Agent{};

        rxcpp::observable<VpnManagementConnectionStatus> ConnectStatusChange$;
        rxcpp::subjects::subject<VpnManagementConnectionStatus> ManualManagement$;

        IAsyncOperation<VpnManagementErrorStatus> Connect();
        IAsyncOperation<VpnManagementErrorStatus> Disconnect();

      private:
        VpnPlugInProfile m_profile;
    };
}
