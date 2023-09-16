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

    hstring SplitRoutingRulesetControl::RulesetName()
    {
        return GetValue(m_rulesetNameProperty).as<hstring>();
    }
    void SplitRoutingRulesetControl::RulesetName(hstring const &value)
    {
        SetValue(m_rulesetNameProperty, box_value(value));
    }
    bool SplitRoutingRulesetControl::CanModifyRuleList()
    {
        return unbox_value<bool>(GetValue(m_canModifyRuleListProperty));
    }
    void SplitRoutingRulesetControl::CanModifyRuleList(bool value)
    {
        SetValue(m_canModifyRuleListProperty, box_value(value));
    }

    IObservableVector<YtFlowApp::SplitRoutingRuleModel> SplitRoutingRulesetControl::RuleList()
    {
        return GetValue(m_ruleListProperty).as<IObservableVector<YtFlowApp::SplitRoutingRuleModel>>();
    }
    void SplitRoutingRulesetControl::RuleList(IObservableVector<YtFlowApp::SplitRoutingRuleModel> const &value)
    {
        SetValue(m_ruleListProperty, value);
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
}
