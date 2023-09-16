#pragma once

#include "SplitRoutingRulesetControl.g.h"

#include "SplitRoutingRuleModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct SplitRoutingRulesetControl : SplitRoutingRulesetControlT<SplitRoutingRulesetControl>
    {
        SplitRoutingRulesetControl()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        static Windows::UI::Xaml::DependencyProperty RulesetNameProperty();
        static Windows::UI::Xaml::DependencyProperty CanModifyRuleListProperty();
        static Windows::UI::Xaml::DependencyProperty RuleListProperty();

        hstring RulesetName();
        void RulesetName(hstring const &value);
        bool CanModifyRuleList();
        void CanModifyRuleList(bool value);
        Windows::Foundation::Collections::IObservableVector<YtFlowApp::SplitRoutingRuleModel> RuleList();
        void RuleList(
            Windows::Foundation::Collections::IObservableVector<YtFlowApp::SplitRoutingRuleModel> const &value);
        event_token RemoveRequested(
            Windows::Foundation::TypedEventHandler<YtFlowApp::SplitRoutingRulesetControl,
                                                   Windows::Foundation::IInspectable> const &handler);

        void RemoveRequested(event_token const &token) noexcept;

      private:
        inline static Windows::UI::Xaml::DependencyProperty m_rulesetNameProperty =
            Windows::UI::Xaml::DependencyProperty::Register(
                L"RulesetName", winrt::xaml_typename<hstring>(),
                winrt::xaml_typename<YtFlowApp::SplitRoutingRulesetControl>(), nullptr);
        inline static Windows::UI::Xaml::DependencyProperty m_canModifyRuleListProperty =
            Windows::UI::Xaml::DependencyProperty::Register(
                L"CanModifyRuleList", winrt::xaml_typename<bool>(),
                winrt::xaml_typename<YtFlowApp::SplitRoutingRulesetControl>(), nullptr);
        inline static Windows::UI::Xaml::DependencyProperty m_ruleListProperty =
            Windows::UI::Xaml::DependencyProperty::Register(
                L"RuleList",
                winrt::xaml_typename<Windows::Foundation::Collections::IObservableVector<SplitRoutingRuleModel>>(),
                winrt::xaml_typename<YtFlowApp::SplitRoutingRulesetControl>(), nullptr);

        event<Windows::Foundation::TypedEventHandler<YtFlowApp::SplitRoutingRulesetControl,
                                                     Windows::Foundation::IInspectable>>
            m_removeRequested;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct SplitRoutingRulesetControl
        : SplitRoutingRulesetControlT<SplitRoutingRulesetControl, implementation::SplitRoutingRulesetControl>
    {
    };
}
