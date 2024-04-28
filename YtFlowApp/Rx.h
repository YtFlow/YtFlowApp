#pragma once

#include <rxcpp/rx.hpp>

namespace winrt::YtFlowApp::implementation
{
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

        template <typename I, typename R = decltype(std::declval<I>().get())>
        static auto observe_awaitable(I &&awaitable)
        {
            return rxcpp::observable<>::create<R>([awaitable = std::forward<I>(awaitable)](rxcpp::subscriber<R> s) {
                auto cb = [](auto awaitable, auto s) mutable -> concurrency::task<void> {
                    try
                    {
                        s.on_next(co_await std::move(awaitable));
                        s.on_completed();
                    }
                    catch (...)
                    {
                        s.on_error(std::current_exception());
                    }
                };
                cb(awaitable, s);
            });
        }

        template <typename I, typename R = decltype(std::declval<I>().get())>
            requires std::is_void_v<R>
        static auto observe_awaitable(I &&awaitable)
        {
            return rxcpp::observable<>::create<R>([awaitable = std::forward<I>(awaitable)](rxcpp::subscriber<R> s) {
                auto cb = [](auto awaitable, auto s) mutable -> concurrency::task<void> {
                    try
                    {
                        s.on_next(co_await std::move(awaitable));
                        s.on_completed();
                    }
                    catch (...)
                    {
                        s.on_error(std::current_exception());
                    }
                };
                cb(awaitable, s);
            });
        }
    };
    rxcpp::observable<bool> ObserveApplicationEnteredBackground();
    rxcpp::observable<bool> ObserveApplicationLeavingBackground();
}
