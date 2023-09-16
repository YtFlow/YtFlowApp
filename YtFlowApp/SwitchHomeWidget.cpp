#include "pch.h"
#include "SwitchHomeWidget.h"
#if __has_include("SwitchHomeWidget.g.cpp")
#include "SwitchHomeWidget.g.cpp"
#endif

#include "CoreRpc.h"
#include "SwitchChoiceItem.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;
using namespace Primitives;

namespace winrt::YtFlowApp::implementation
{
    SwitchHomeWidget::SwitchHomeWidget()
    {
        InitializeComponent();
    }
    SwitchHomeWidget::SwitchHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo,
                                       RequestSender sendRequest)
        : m_sharedInfo(std::move(sharedInfo)), m_sendRequest(std::move(sendRequest))
    {
        InitializeComponent();

        PluginNameText().Text(std::move(pluginName));
    }

    void SwitchHomeWidget::UpdateInfo()
    {
        SwitchInfo info = nlohmann::json::from_cbor(*m_sharedInfo);
        if (info.current >= info.choices.size())
        {
            return;
        }

        PreviewSelectionNameText().Text(to_hstring(info.choices[info.current].name));

        auto const items{SwitchList().Items()};
        items.Clear();
        for (auto const &choice : info.choices)
        {
            auto const item{winrt::make<SwitchChoiceItem>()};
            item.Name(to_hstring(choice.name));
            item.Description(to_hstring(choice.description));
            if (info.current-- == 0)
            {
                item.IsActive(true);
            }
            items.Append(item);
        }
    }

    fire_and_forget SwitchHomeWidget::ChoiceToggleButton_Checked(IInspectable const &sender,
                                                                 RoutedEventArgs const & /* e */)
    {
        try
        {
            auto const itemObj{sender.as<FrameworkElement>().DataContext()};
            if (itemObj == nullptr)
            {
                co_return;
            }
            auto const item{get_self<SwitchChoiceItem>(itemObj.as<YtFlowApp::SwitchChoiceItem>())};
            if (item->IsActive())
            {
                co_return;
            }

            uint32_t idx{};
            if (!SwitchList().Items().IndexOf(itemObj, idx))
            {
                co_return;
            }
            for (auto const otherObj : SwitchList().Items())
            {
                auto const otherItem{otherObj.as<YtFlowApp::SwitchChoiceItem>()};
                if (otherItem.IsActive())
                {
                    otherItem.IsActive(false);
                    break;
                }
            }
            item->IsActive(true);

            auto const lifetime{get_strong()};
            co_await resume_background();
            nlohmann::json doc = idx;
            co_await lifetime->m_sendRequest("s", nlohmann::json::to_cbor(doc));
        }
        catch (...)
        {
            NotifyException(L"Switch");
        }
    }
    void SwitchHomeWidget::ChoiceToggleButton_Unchecked(IInspectable const &sender, RoutedEventArgs const & /* e */)
    {
        auto const btn{sender.as<ToggleButton>()};
        if (get_self<SwitchChoiceItem>(btn.DataContext().as<YtFlowApp::SwitchChoiceItem>())->IsActive())
        {
            btn.IsChecked(true);
        }
    }
}
