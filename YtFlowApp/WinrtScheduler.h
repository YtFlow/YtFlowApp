#pragma once

#include <rxcpp/rx.hpp>

using winrt::Windows::UI::Core::CoreDispatcher;

namespace winrt::YtFlowApp::implementation
{
    struct RxDispatcherScheduler : public rxcpp::schedulers::scheduler_interface
    {
        struct dispatcher_worker : public rxcpp::schedulers::worker_interface
        {
            dispatcher_worker(rxcpp::composite_subscription cs, CoreDispatcher dispatcher)
                : lifetime(cs), m_dispatcher(dispatcher)
            {
            }

            virtual clock_type::time_point now() const override;
            virtual void schedule(const rxcpp::schedulers::schedulable &scbl) const override;
            virtual void schedule(clock_type::time_point when,
                                  const rxcpp::schedulers::schedulable &scbl) const override;

          private:
            CoreDispatcher m_dispatcher;
            std::shared_ptr<std::atomic<uint32_t>> m_workCount = std::make_shared<std::atomic<uint32_t>>(0);
            rxcpp::composite_subscription lifetime;
        };

        RxDispatcherScheduler(CoreDispatcher dispatcher) : Dispatcher(dispatcher)
        {
        }
        virtual clock_type::time_point now() const override;
        virtual rxcpp::schedulers::worker create_worker(rxcpp::composite_subscription cs) const override;
        CoreDispatcher Dispatcher;
    };
    struct RxWinrtThreadPoolScheduler : public rxcpp::schedulers::scheduler_interface
    {
        struct dispatcher_worker : public rxcpp::schedulers::worker_interface
        {
            dispatcher_worker(rxcpp::composite_subscription cs) : lifetime(cs)
            {
            }

            virtual clock_type::time_point now() const override;
            virtual void schedule(const rxcpp::schedulers::schedulable &scbl) const override;
            virtual void schedule(clock_type::time_point when,
                                  const rxcpp::schedulers::schedulable &scbl) const override;

          private:
            std::shared_ptr<std::atomic<uint32_t>> m_workCount = std::make_shared<std::atomic<uint32_t>>(0);
            rxcpp::composite_subscription lifetime;
        };

        RxWinrtThreadPoolScheduler()
        {
        }
        virtual clock_type::time_point now() const override;
        virtual rxcpp::schedulers::worker create_worker(rxcpp::composite_subscription cs) const override;
    };
    rxcpp::observe_on_one_worker ObserveOnDispatcher(CoreDispatcher dispatcher = {nullptr});
    rxcpp::observe_on_one_worker ObserveOnWinrtThreadPool();
}
