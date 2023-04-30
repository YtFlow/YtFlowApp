#include "pch.h"
#include "WinrtScheduler.h"

using namespace rxcpp::schedulers;
using winrt::Windows::Foundation::DateTime;
using winrt::Windows::Foundation::TimeSpan;
using winrt::Windows::UI::Core::CoreDispatcherPriority;

namespace winrt::YtFlowApp::implementation
{
    worker_interface::clock_type::time_point RxDispatcherScheduler::dispatcher_worker::now() const
    {
        return worker_interface::clock_type::now();
    }
    void RxDispatcherScheduler::dispatcher_worker::schedule(const schedulable &scbl) const
    {
        if (!scbl.is_subscribed())
        {
            return;
        }
        m_dispatcher.RunAsync(CoreDispatcherPriority::Normal,
                              [scbl, weakCount = static_cast<std::weak_ptr<std::atomic<uint32_t>>>(m_workCount)] {
                                  auto strong_count = weakCount.lock();
                                  if (strong_count == nullptr)
                                  {
                                      return;
                                  }
                                  recursion r(strong_count->fetch_sub(1) == 1);
                                  scbl(r.get_recurse());
                              });
        m_workCount->fetch_add(1);
    }
    void RxDispatcherScheduler::dispatcher_worker::schedule(worker_interface::clock_type::time_point when,
                                                            const schedulable &scbl) const
    {
        if (!scbl.is_subscribed())
        {
            return;
        }
        const auto run = [](auto when, auto scbl, auto const dispatcher, auto weakCount) -> winrt::fire_and_forget {
            auto timeSpan = when - worker_interface::clock_type::now();
            // Ensure lifetime of captured variables
            co_await winrt::resume_after(std::chrono::duration_cast<TimeSpan>(timeSpan));
            co_await winrt::resume_foreground(dispatcher);
            auto strong_count = weakCount.lock();
            if (strong_count == nullptr)
            {
                co_return;
            }
            recursion r(strong_count->fetch_sub(1) == 1);
            scbl(r.get_recurse());
        };
        run(when, scbl, m_dispatcher, static_cast<std::weak_ptr<std::atomic<uint32_t>>>(m_workCount));
        m_workCount->fetch_add(1);
    }

    worker_interface::clock_type::time_point RxDispatcherScheduler::now() const
    {
        return worker_interface::clock_type::now();
    }

    worker RxDispatcherScheduler::create_worker(rxcpp::composite_subscription cs) const
    {
        return worker(cs, std::make_shared<dispatcher_worker>(cs, Dispatcher));
    }

    worker_interface::clock_type::time_point RxWinrtThreadPoolScheduler::dispatcher_worker::now() const
    {
        return worker_interface::clock_type::now();
    }
    void RxWinrtThreadPoolScheduler::dispatcher_worker::schedule(const schedulable &scbl) const
    {
        if (!scbl.is_subscribed())
        {
            return;
        }
        const auto run = [](auto scbl, auto weakCount) -> winrt::fire_and_forget {
            // Ensure lifetime of captured variables
            co_await resume_background();
            auto strong_count = weakCount.lock();
            if (strong_count == nullptr)
            {
                co_return;
            }
            recursion const r(strong_count->fetch_sub(1) == 1);
            scbl(r.get_recurse());
        };
        run(scbl, static_cast<std::weak_ptr<std::atomic<uint32_t>>>(m_workCount));
        m_workCount->fetch_add(1);
    }
    void RxWinrtThreadPoolScheduler::dispatcher_worker::schedule(worker_interface::clock_type::time_point when,
                                                                 const schedulable &scbl) const
    {
        if (!scbl.is_subscribed())
        {
            return;
        }
        const auto run = [](auto when, auto scbl, auto weakCount) -> winrt::fire_and_forget {
            auto timeSpan = when - worker_interface::clock_type::now();
            co_await winrt::resume_after(std::chrono::duration_cast<TimeSpan>(timeSpan));
            auto strong_count = weakCount.lock();
            if (strong_count == nullptr)
            {
                co_return;
            }
            recursion const r(strong_count->fetch_sub(1) == 1);
            scbl(r.get_recurse());
        };
        run(when, scbl, static_cast<std::weak_ptr<std::atomic<uint32_t>>>(m_workCount));
        m_workCount->fetch_add(1);
    }

    worker_interface::clock_type::time_point RxWinrtThreadPoolScheduler::now() const
    {
        return worker_interface::clock_type::now();
    }

    worker RxWinrtThreadPoolScheduler::create_worker(rxcpp::composite_subscription cs) const
    {
        return worker(cs, std::make_shared<dispatcher_worker>(cs));
    }

    rxcpp::observe_on_one_worker ObserveOnDispatcher(CoreDispatcher dispatcher)
    {
        static rxcpp::observe_on_one_worker r(rxcpp::schedulers::make_scheduler<RxDispatcherScheduler>(dispatcher));
        return r;
    }
    rxcpp::observe_on_one_worker ObserveOnWinrtThreadPool()
    {
        static rxcpp::observe_on_one_worker r(rxcpp::schedulers::make_scheduler<RxWinrtThreadPoolScheduler>());
        return r;
    }
}
