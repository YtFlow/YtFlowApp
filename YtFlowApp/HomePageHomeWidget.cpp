#include "pch.h"
#include "HomePage.h"

#include "DynOutboundHomeWidget.h"
#include "ForwardHomeWidget.h"
#include "NetifHomeWidget.h"
#include "SwitchHomeWidget.h"

namespace winrt::YtFlowApp::implementation
{
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
        else if (info.plugin == "forward")
        {
            auto widget = winrt::make<ForwardHomeWidget>(to_hstring(info.name), handle.info);
            handle.widget = widget;
            ConnectedViewSidePanel().Children().Append(std::move(widget));
        }
        else
        {
            return {std::nullopt};
        }
        return handle;
    }
}
