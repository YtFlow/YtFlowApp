#include "pch.h"
#include "SplitRoutingRuleModel.h"
#include "SplitRoutingRuleModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;

    hstring SplitRoutingRuleModel::Rule()
    {
        return m_rule;
    }
    void SplitRoutingRuleModel::Rule(hstring const &value)
    {
        m_rule = value;
        m_propertyChanged(*this, PropertyChangedEventArgs{L"Rule"});
    }

    SplitRoutingRuleDecision SplitRoutingRuleModel::Decision() const
    {
        return m_decision;
    }
    void SplitRoutingRuleModel::Decision(SplitRoutingRuleDecision const &value)
    {
        if (value == m_decision)
        {
            return;
        }
        m_decision = value;
        m_propertyChanged(*this, PropertyChangedEventArgs{L"Decision"});
        m_propertyChanged(*this, PropertyChangedEventArgs{L"DecisionIndex"});
    }
    int32_t SplitRoutingRuleModel::DecisionIndex() const noexcept
    {
        return static_cast<int32_t>(m_decision);
    }
    void SplitRoutingRuleModel::DecisionIndex(int32_t value)
    {
        Decision(static_cast<SplitRoutingRuleDecision>(value));
    }

    event_token SplitRoutingRuleModel::PropertyChanged(
        Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void SplitRoutingRuleModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
