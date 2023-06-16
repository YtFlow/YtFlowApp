#pragma once

#include "EditProfilePage.g.h"

#include "EditPluginModel.h"
#include "ProfileModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditProfilePage : EditProfilePageT<EditProfilePage>
    {
        enum class SortType
        {
            ByName,
            ByDependency,
            ByCategory,
        };

        EditProfilePage();

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        fire_and_forget OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void CheckRenamingPlugin(EditPluginModel *editPluginModel) const &;
        com_ptr<YtFlowApp::implementation::EditPluginModel> CreateEditPluginModel(FfiPlugin const &plugin,
                                                                                  bool isEntry);

        void AdaptiveWidth_StateChanged(Windows::Foundation::IInspectable const &sender,
                                        Windows::UI::Xaml::VisualStateChangedEventArgs const &e);
        void ProfileNameBox_KeyDown(IInspectable const &sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const &e);
        void ProfileNameBox_LostFocus(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_ItemInvoked(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                        Microsoft::UI::Xaml::Controls::TreeViewItemInvokedEventArgs const &args);
        void SortByItem_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_ExpandingDeps(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                          Microsoft::UI::Xaml::Controls::TreeViewExpandingEventArgs const &args);
        fire_and_forget SetAsEntryMenuItem_Click(IInspectable const &sender,
                                                 Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget DeactivateMenuItem_Click(IInspectable const &sender,
                                                 Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_Collapsed(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                      Microsoft::UI::Xaml::Controls::TreeViewCollapsedEventArgs const &args);
        fire_and_forget DeleteMenuItem_Click(Windows::Foundation::IInspectable const &sender,
                                             Windows::UI::Xaml::RoutedEventArgs const &e);
        fire_and_forget AddPluginButton_Click(Windows::Foundation::IInspectable const &sender,
                                              Windows::UI::Xaml::RoutedEventArgs const &e);
        void NewPluginNameText_TextChanged(Windows::Foundation::IInspectable const &sender,
                                           Windows::UI::Xaml::Controls::TextChangedEventArgs const &e);
        void AddPluginDialog_Closing(Windows::UI::Xaml::Controls::ContentDialog const &sender,
                                     Windows::UI::Xaml::Controls::ContentDialogClosingEventArgs const &args);

      private:
        static inline Windows::Storage::ApplicationData appData{Windows::Storage::ApplicationData::Current()};

        Windows::Foundation::IAsyncAction SaveProfileName();
        void RefreshTreeView();
        void LoadTreeNodesByName();
        void LoadTreeNodesByDependency();
        void LoadTreeNodesByCategory();

        bool m_forceQuit{false};
        com_ptr<ProfileModel> m_profile{nullptr};
        std::vector<com_ptr<YtFlowApp::implementation::EditPluginModel>> m_pluginModels;
        SortType m_sortType{appData.LocalSettings()
                                .Values()
                                .TryLookup(L"YTFLOW_APP_PROFILE_EDIT_PLUGIN_SORT_TYPE")
                                .try_as<int32_t>()
                                .value_or(static_cast<int32_t>(SortType::ByName))};
        event_token m_treeViewExpanding{};
        rxcpp::subjects::subject<bool> m_depChangeSubject$;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProfilePage : EditProfilePageT<EditProfilePage, implementation::EditProfilePage>
    {
    };
}
