#include "pch.h"
#include "VpnTask.h"
#include "VpnTask.g.cpp"

namespace winrt::YtFlowVpnTask::implementation
{
    VpnTask::VpnTask()
    {
        if (PluginInstance == nullptr) {
            std::lock_guard<std::mutex> _guard{ PluginInstanceCtorLock };
            if (PluginInstance == nullptr) {
                void* pluginAbi;
                if (CreateVpnPlugIn(&pluginAbi) != 0) {
                    throw L"Cannot create plugin";
                }
                winrt::attach_abi(PluginInstance, pluginAbi);
            }
        }
    }
    void VpnTask::Run(winrt::Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance)
    {
        winrt::Windows::Networking::Vpn::VpnChannel::ProcessEventAsync(PluginInstance, taskInstance.TriggerDetails());
    }
}
