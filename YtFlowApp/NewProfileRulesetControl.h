#pragma once

#include "NewProfileRulesetControl.g.h"

#include "winrt\Windows.Web.Http.Filters.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct NewProfileRulesetControl : NewProfileRulesetControlT<NewProfileRulesetControl>
    {
        NewProfileRulesetControl() = default;

        bool RulesetSelected();
        hstring RulesetName();
        std::string_view GetResourceKeyFromSelectedRuleset();
        fire_and_forget InitSelectedRuleset();
        void SelectionComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const &sender,
                                                winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const &e);
        void ContentDialog_Opened(winrt::Windows::UI::Xaml::Controls::ContentDialog const &sender,
                                  winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs const &args);
        void ContentDialog_Closing(winrt::Windows::UI::Xaml::Controls::ContentDialog const &sender,
                                   winrt::Windows::UI::Xaml::Controls::ContentDialogClosingEventArgs const &args);
        void CancelUpdateButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                      winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget UpdateButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                           winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ContentDialog_PrimaryButtonClick(
            winrt::Windows::UI::Xaml::Controls::ContentDialog const &sender,
            winrt::Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const &args);

      private:
        static inline Windows::Storage::StorageFolder m_resourceFolder{nullptr};

        bool m_rulesetSelected{false};
        hstring m_rulesetName;
        Windows::Web::Http::HttpClient m_client{nullptr};
        event_token m_selectionChangeToken;
        std::vector<FfiResource> m_resources;
        bool m_updating{false};
        std::atomic_bool m_updateCancelled{false};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct NewProfileRulesetControl
        : NewProfileRulesetControlT<NewProfileRulesetControl, implementation::NewProfileRulesetControl>
    {
    };
}
