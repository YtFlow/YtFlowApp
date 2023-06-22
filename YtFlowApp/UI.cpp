#include "pch.h"
#include "UI.h"

#include "WinrtScheduler.h"

using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::UI::Xaml::Input;
using namespace winrt::Windows::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    void NotifyUser(hstring msg, hstring title, Windows::UI::Core::CoreDispatcher inputDispatcher)
    {
        static Windows::UI::Core::CoreDispatcher dispatcher{nullptr};
        if (inputDispatcher != nullptr)
        {
            dispatcher = std::move(inputDispatcher);
            return;
        }
        static std::vector<std::pair<hstring, hstring>> messages{};
        dispatcher.RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                            [msg = std::move(msg), title = std::move(title)]() -> winrt::fire_and_forget {
                                messages.push_back(std::make_pair(msg, title));

                                static bool isQueueRunning{false};
                                if (std::exchange(isQueueRunning, true))
                                {
                                    co_return;
                                }

                                // Display all messages until the queue is drained.
                                while (messages.size() > 0)
                                {
                                    auto it = messages.begin();
                                    auto [message, messageTitle] = std::move(*it);
                                    messages.erase(it);
                                    ContentDialog dialog;
                                    if (messageTitle.size() > 0)
                                    {
                                        dialog.Title(box_value(std::move(messageTitle)));
                                    }
                                    if (ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Input.StandardUICommand"))
                                    {
                                        dialog.CloseButtonCommand(StandardUICommand(StandardUICommandKind::Close));
                                    }
                                    else if (ApiInformation::IsPropertyPresent(
                                                 L"Windows.UI.Xaml.Controls.ContentDialog", L"CloseButtonText"))
                                    {
                                        dialog.CloseButtonText(L"Close");
                                    }
                                    else
                                    {
                                        dialog.SecondaryButtonText(L"Close");
                                    }
                                    dialog.Content(box_value(message));

                                    try
                                    {
                                        co_await dialog.ShowAsync();
                                    }
                                    catch (...)
                                    {
                                        // Really don't know what to do here (e.g. there is an existing dialog)
                                    }
                                }
                                isQueueRunning = false;
                            });
    }

    void NotifyUser(char const *msg, hstring title)
    {
        NotifyUser(to_hstring(msg), std::move(title));
    }

    void NotifyException(std::wstring_view const context)
    {
        try
        {
            throw;
        }
        catch (hresult_error const &hr)
        {
            NotifyUser(hr.message(), hstring{L"Error occurred: "} + context);
        }
        catch (std::exception const &ex)
        {
            NotifyUser(ex.what(), hstring{L"Unexpected exception: "} + context);
        }
    }

    hstring HumanizeByte(uint64_t num)
    {
        if (num == 0)
        {
            return L"0 B";
        }
        if (num < 1024)
        {
            return to_hstring(num) + L" B";
        }
        if (num < 1024ULL * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024) / 10) + L" KB";
        }
        if (num < 1024ULL * 1024 * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024 / 1024) / 10) + L" MB";
        }
        if (num < 1024ULL * 1024 * 1024 * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024 / 1024 / 1024) / 10) + L" GB";
        }
        if (num < 1024ULL * 1024 * 1024 * 1024 * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024 / 1024 / 1024 / 1024) / 10) + L" TB";
        }
        return L"∞";
    }
    hstring HumanizeByteSpeed(uint64_t num)
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
            return to_hstring(static_cast<double>(num * 10 / 1024) / 10) + L" KB/s";
        }
        if (num < 1024ULL * 1024 * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024 / 1024) / 10) + L" MB/s";
        }
        if (num < 1024ULL * 1024 * 1024 * 1000)
        {
            return to_hstring(static_cast<double>(num * 10 / 1024 / 1024 / 1024) / 10) + L" GB/s";
        }
        return L"∞";
    }

    hstring FormatNaiveDateTime(char const *dateStr)
    {
        if (dateStr == nullptr)
        {
            return L"";
        }

        std::istringstream ss{dateStr};

        using namespace std::chrono;
        sys_seconds tp;
        time_zone const *tz{};
        try
        {
            tz = current_zone();
        }
        catch (std::runtime_error const &)
        {
        }
        if (tz == nullptr || !(ss >> parse("%Y-%m-%dT%H:%M:%S", tp)))
        {
            return L"";
        }

        auto const ltp = tz->to_local(tp);
        return to_hstring(std::format("{:%Y-%m-%d %H:%M:%S}", ltp));
    }
}