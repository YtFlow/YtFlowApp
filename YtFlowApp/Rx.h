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
                    winrt::event_token token = r([t, s](auto &&...Args) {
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

        template <typename I, typename R = decltype(std::declval<I>().GetResults())>
        static auto observe_awaitable(I &&awaitable)
        {
            return rxcpp::observable<>::create<R>([awaitable = std::move(awaitable)](rxcpp::subscriber<R> s) mutable {
                auto cb = [awaitable = std::move(awaitable), s = std::move(s)]() -> winrt::fire_and_forget {
                    try
                    {
                        s.on_next(co_await awaitable);
                        s.on_completed();
                    }
                    catch (...)
                    {
                        s.on_error(std::current_exception());
                    }
                };
                cb();
            });
        }

        template <typename I, typename R = decltype(std::declval<I>().GetResults()),
                  typename _e = std::enable_if_t<std::is_void<R>()>>
        static auto observe_awaitable(I &&awaitable)
        {
            return rxcpp::observable<>::create<R>([awaitable = std::move(awaitable)](rxcpp::subscriber<R> s) {
                auto cb = [awaitable = std::move(awaitable), s = std::move(s)]() -> winrt::fire_and_forget {
                    try
                    {
                        s.on_next(co_await awaitable);
                        s.on_completed();
                    }
                    catch (...)
                    {
                        s.on_error(std::current_exception());
                    }
                };
                cb();
            });
        }

    };
    rxcpp::observable<bool> ObserveApplicationEnteredBackground();
    rxcpp::observable<bool> ObserveApplicationLeavingBackground();
}
