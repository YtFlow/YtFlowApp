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
        static Windows::UI::Xaml::DependencyProperty FallbackRuleProperty();

        hstring RulesetName() const;
        void RulesetName(hstring const &value) const;
        bool CanModifyRuleList() const;
        void CanModifyRuleList(bool value) const;
        Windows::Foundation::Collections::IObservableVector<YtFlowApp::SplitRoutingRuleModel> RuleList() const;
        void RuleList(
            Windows::Foundation::Collections::IObservableVector<YtFlowApp::SplitRoutingRuleModel> const &value) const;
        YtFlowApp::SplitRoutingRuleModel FallbackRule() const;
        void FallbackRule(YtFlowApp::SplitRoutingRuleModel const &model) const;

        void DeleteRulesetButton_Click(Windows::Foundation::IInspectable const &sender,
                                       Windows::UI::Xaml::RoutedEventArgs const &e);
        void AddRuleButton_Click(Windows::Foundation::IInspectable const &sender,
                                 Windows::UI::Xaml::RoutedEventArgs const &e) const;
        void RemoveRuleButton_Click(Windows::Foundation::IInspectable const &sender,
                                    Windows::UI::Xaml::RoutedEventArgs const &e);

        Windows::UI::Xaml::Controls::ListViewSelectionMode CanModifyToListSelectionMode(bool canModify) const noexcept;

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
        inline static Windows::UI::Xaml::DependencyProperty m_fallbackRuleProperty =
            Windows::UI::Xaml::DependencyProperty::Register(
                L"FallbackRule", winrt::xaml_typename<YtFlowApp::SplitRoutingRuleModel>(),
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
