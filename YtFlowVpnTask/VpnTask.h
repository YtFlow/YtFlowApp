#pragma once
#include <cstdint>
#include <mutex>
#include "VpnTask.g.h"

using winrt::Windows::Networking::Vpn::IVpnPlugIn;
extern "C" {
    int32_t CreateVpnPlugIn(void** plugin);
}

namespace winrt::YtFlowVpnTask::implementation
{
    struct VpnTask : VpnTaskT<VpnTask>
    {
        VpnTask();

        void Run(winrt::Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance);

    private:
        inline static std::mutex PluginInstanceCtorLock;
        inline static IVpnPlugIn PluginInstance{ nullptr };
    };
}
namespace winrt::YtFlowVpnTask::factory_implementation
{
    struct VpnTask : VpnTaskT<VpnTask, implementation::VpnTask>
    {
    };
}
