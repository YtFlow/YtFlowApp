#include "pch.h"
#include "ForwardHomeWidget.h"
#if __has_include("ForwardHomeWidget.g.cpp")
#include "ForwardHomeWidget.g.cpp"
#endif

#include "Rx.h"
#include "UI.h"
#include "WinrtScheduler.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    hstring ForwardHomeWidget::HumanizeByteSpeed(uint64_t num)
    {
        if (num == 0)
        {
            return L"0 B/s";
        }
        if (num < 1024)
        {
            return to_hstring(num) + L" B/s";
        }
        if (num < 1024ULL * 1000)
        {
            return to_hstring(double(num * 10 / 1024) / 10) + L" KB/s";
        }
        if (num < 1024ULL * 1024 * 1000)
        {
            return to_hstring(double(num * 10 / 1024 / 1024) / 10) + L" MB/s";
        }
        if (num < 1024ULL * 1024 * 1024 * 1000)
        {
            return to_hstring(double(num * 10 / 1024 / 1024 / 1024) / 10) + L" GB/s";
        }
        return L"∞";
    }

    ForwardHomeWidget::ForwardHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo)
        : m_sharedInfo(std::move(sharedInfo))
    {
        InitializeComponent();

        PluginNameText().Text(std::move(pluginName));
    }

    void ForwardHomeWidget::UpdateInfo()
    {
        try
        {
            nlohmann::json doc = nlohmann::json::from_cbor(*m_sharedInfo);
            ForwardStatSnapshot snapshot{.uplink_written = doc.at("uplink_written"),
                                         .downlink_written = doc.at("downlink_written"),
                                         .tcp_connections = doc.at("tcp_connection_count"),
                                         .udp_sessions = doc.at("udp_session_count")};
            m_lastStat = snapshot;
        }
        catch (...)
        {
            NotifyException(L"Updating Forward");
        }
    }
    void ForwardHomeWidget::UserControl_Loaded(IInspectable const &, RoutedEventArgs const &)
    {
        using namespace std::literals::chrono_literals;

        auto const weak{get_weak()};
        auto lastTimestamp = std::make_shared<std::chrono::system_clock::time_point>(std::chrono::system_clock::now());
        auto lastStat = std::make_shared<ForwardStatSnapshot>(m_lastStat);
        auto const focus${ObserveApplicationLeavingBackground()};
        auto const unfocus${ObserveApplicationEnteredBackground()};
        m_renderStat$ =
            focus$.start_with(true)
                .flat_map([=](auto) {
                    auto const self{weak.get()};
                    if (!self)
                    {
                        throw std::runtime_error("Cannot subscribe when ForwardHomeWidget disposed");
                    }
                    return rxcpp::observable<>::interval(1s)
                        .map([](auto) { return true; })
                        .subscribe_on(ObserveOnWinrtThreadPool())
                        .take_until(unfocus$);
                })
                .subscribe_on(ObserveOnWinrtThreadPool())
                .observe_on(ObserveOnDispatcher())
                .subscribe(
                    [weak, lastTimestamp, lastStat](auto) {
                        auto const self{weak.get()};
                        if (!self)
                        {
                            return;
                        }

                        auto const now = std::chrono::system_clock::now();
                        if (now == *lastTimestamp)
                        {
                            return;
                        }

                        try
                        {
                            auto const &stat = self->m_lastStat;
                            self->TcpCountText().Text(to_hstring(stat.tcp_connections));
                            self->UdpCountText().Text(to_hstring(stat.udp_sessions));
                            auto const timespan = now - *lastTimestamp;
                            hstring uplinkText, downlinkText;
                            if (timespan > 3s)
                            {
                                uplinkText = HumanizeByteSpeed(0);
                                downlinkText = HumanizeByteSpeed(0);
                            }
                            else if (timespan < 1005ms)
                            {
                                uplinkText = HumanizeByteSpeed(stat.uplink_written - lastStat->uplink_written);
                                downlinkText = HumanizeByteSpeed(stat.downlink_written - lastStat->downlink_written);
                            }
                            else
                            {
                                auto const scale =
                                    static_cast<double>(
                                        std::chrono::duration_cast<std::chrono::milliseconds>(timespan).count()) /
                                    1000;
                                uplinkText = HumanizeByteSpeed(
                                    static_cast<uint64_t>((stat.uplink_written - lastStat->uplink_written) / scale));
                                downlinkText = HumanizeByteSpeed(static_cast<uint64_t>(
                                    (stat.downlink_written - lastStat->downlink_written) / scale));
                            }
                            self->UplinkText().Text(uplinkText);
                            self->DownlinkText().Text(downlinkText);
                            *lastStat = stat;
                            *lastTimestamp = now;
                        }
                        catch (...)
                        {
                            NotifyException(L"ForwardHomeWidget Stat subscribe:");
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
                            NotifyException(L"ForwardHomeWidget Stat subscribe error");
                        }
                    });
    }

    void ForwardHomeWidget::UserControl_Unloaded(IInspectable const &, RoutedEventArgs const &)
    {
        m_renderStat$.unsubscribe();
    }
}
