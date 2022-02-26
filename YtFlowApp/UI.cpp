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

                                    co_await dialog.ShowAsync();
                                }
                                isQueueRunning = false;
                            });
    }
}