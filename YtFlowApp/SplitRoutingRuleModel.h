#pragma once
#include "SplitRoutingRuleModel.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct SplitRoutingRuleModel : SplitRoutingRuleModelT<SplitRoutingRuleModel>
    {
        SplitRoutingRuleModel() = default;
        SplitRoutingRuleModel(hstring rule, SplitRoutingRuleDecision decision)
            : m_rule(std::move(rule)), m_decision(decision)
        {
        }

        hstring Rule();
        void Rule(hstring const &value);
        SplitRoutingRuleDecision Decision() const;
        void Decision(SplitRoutingRuleDecision const &value);
        int32_t DecisionIndex() const noexcept;
        void DecisionIndex(int32_t value);
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

      private:
        hstring m_rule;
        SplitRoutingRuleDecision m_decision{};
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct SplitRoutingRuleModel : SplitRoutingRuleModelT<SplitRoutingRuleModel, implementation::SplitRoutingRuleModel>
    {
    };
}
