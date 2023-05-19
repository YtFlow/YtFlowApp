#include "pch.h"
#include "HomePage.h"

#include "CoreRpc.h"
#include "Rx.h"
#include "UI.h"
#include "WinrtScheduler.h"

namespace winrt::YtFlowApp::implementation
{
    using namespace std::literals::chrono_literals;

    void HomePage::SubscribeRefreshPluginStatus()
    {
        m_refreshPluginStatus$.unsubscribe();
        PluginWidgetPanel().Children().Clear();
        ConnectedViewSidePanel().Children().Clear();
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
                                     catch (...)
                                     {
                                         NotifyException(L"RPC");
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
}
