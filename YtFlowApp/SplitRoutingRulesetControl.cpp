#include "pch.h"
#include "SplitRoutingRulesetControl.h"
#if __has_include("SplitRoutingRulesetControl.g.cpp")
#include "SplitRoutingRulesetControl.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    using Windows::Foundation::Collections::IObservableVector;

    DependencyProperty SplitRoutingRulesetControl::RulesetNameProperty()
    {
        return m_rulesetNameProperty;
    }
    DependencyProperty SplitRoutingRulesetControl::CanModifyRuleListProperty()
    {
        return m_canModifyRuleListProperty;
    }
    DependencyProperty SplitRoutingRulesetControl::RuleListProperty()
    {
        return m_ruleListProperty;
    }
    DependencyProperty SplitRoutingRulesetControl::FallbackRuleProperty()
    {
        return m_fallbackRuleProperty;
    }

    hstring SplitRoutingRulesetControl::RulesetName() const
    {
        return GetValue(m_rulesetNameProperty).as<hstring>();
    }
    void SplitRoutingRulesetControl::RulesetName(hstring const &value) const
    {
        SetValue(m_rulesetNameProperty, box_value(value));
    }
    bool SplitRoutingRulesetControl::CanModifyRuleList() const
    {
        return unbox_value<bool>(GetValue(m_canModifyRuleListProperty));
    }
    void SplitRoutingRulesetControl::CanModifyRuleList(bool value) const
    {
        SetValue(m_canModifyRuleListProperty, box_value(value));
    }

    IObservableVector<YtFlowApp::SplitRoutingRuleModel> SplitRoutingRulesetControl::RuleList() const
    {
        return GetValue(m_ruleListProperty).as<IObservableVector<YtFlowApp::SplitRoutingRuleModel>>();
    }
    void SplitRoutingRulesetControl::RuleList(IObservableVector<YtFlowApp::SplitRoutingRuleModel> const &value) const
    {
        SetValue(m_ruleListProperty, value);
    }

    YtFlowApp::SplitRoutingRuleModel SplitRoutingRulesetControl::FallbackRule() const
    {
        return GetValue(m_fallbackRuleProperty).as<YtFlowApp::SplitRoutingRuleModel>();
    }
    void SplitRoutingRulesetControl::FallbackRule(YtFlowApp::SplitRoutingRuleModel const &model) const
    {
        SetValue(m_fallbackRuleProperty, model);
    }

    Controls::ListViewSelectionMode SplitRoutingRulesetControl::CanModifyToListSelectionMode(
        bool const canModify) const noexcept
    {
        return canModify ? Controls::ListViewSelectionMode::Single : Controls::ListViewSelectionMode::None;
    }

    event_token SplitRoutingRulesetControl::RemoveRequested(
        Windows::Foundation::TypedEventHandler<YtFlowApp::SplitRoutingRulesetControl,
                                               Windows::Foundation::IInspectable> const &handler)
    {
        return m_removeRequested.add(handler);
    }
    void SplitRoutingRulesetControl::RemoveRequested(event_token const &token) noexcept
    {
        m_removeRequested.remove(token);
    }

    void SplitRoutingRulesetControl::DeleteRulesetButton_Click(Windows::Foundation::IInspectable const &,
                                                               RoutedEventArgs const &)
    {
        m_removeRequested(*this, nullptr);
    }
    void SplitRoutingRulesetControl::AddRuleButton_Click(Windows::Foundation::IInspectable const &,
                                                         RoutedEventArgs const &) const
    {
        auto const rule = L"new";
        auto constexpr decision = SplitRoutingRuleDecision::Direct;
        auto model = make<SplitRoutingRuleModel>(rule, decision);
        RuleList().Append(std::move(model));
    }
    void SplitRoutingRulesetControl::RemoveRuleButton_Click(Windows::Foundation::IInspectable const &,
                                                            RoutedEventArgs const &)
    {
        auto const selectedItemsCollection = RuleListView().SelectedItems();
        std::vector const selectedItems(selectedItemsCollection.begin(), selectedItemsCollection.end());
        for (auto const &item : selectedItems)
        {
            auto const model = item.as<SplitRoutingRuleModel>();
            uint32_t index{};
            if (RuleList().IndexOf(*model, index))
            {
                RuleList().RemoveAt(index);
            }
        }
    }
}
