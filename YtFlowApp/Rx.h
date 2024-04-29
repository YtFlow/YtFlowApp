#pragma once

#include <rxcpp/rx.hpp>

namespace winrt::YtFlowApp::implementation
{
    template <typename F, typename R>
    concept ppl_task_creator = requires(F f) {
        {
            f()
        } -> std::same_as<concurrency::task<R>>;
    };

    struct Rx
    {

        template <typename R, typename Register, typename Transform, typename Unregister>
        static auto observe_winrt_event(Register &&r, Transform &&t, Unregister &&d)
        {
            return rxcpp::observable<>::create<R>(
                [r = std::move(r), t = std::move(t), d = std::move(d)](rxcpp::subscriber<R> s) {
                    event_token token = r([t, s](auto &&...Args) {
                        try
                        {
                            s.on_next(std::move(t(Args...)));
                        }
                        catch (...)
                        {
                            s.on_error(std::current_exception());
                        }
                    });
                    s.add([token, d]() { d(token); });
                });
        }

        template <typename F, typename R = decltype(std::declval<F>()())::result_type>
        static auto observe_ppl_task(F factory)
            requires ppl_task_creator<F, R>
        {
            return rxcpp::observable<>::create<R>([factory = std::move(factory)](rxcpp::subscriber<R> subscriber) {
                try
                {
                    factory().then([subscriber](concurrency::task<R> const &task) {
                        try
                        {
                            subscriber.on_next(task.get());
                            subscriber.on_completed();
                        }
                        catch (...)
                        {
                            subscriber.on_error(std::current_exception());
                        }
                    });
                }
                catch (...)
                {
                    subscriber.on_error(std::current_exception());
                }
            });
        }
    };
    rxcpp::observable<bool> ObserveApplicationEnteredBackground();
    rxcpp::observable<bool> ObserveApplicationLeavingBackground();
}
