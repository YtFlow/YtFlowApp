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
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void ProfileNameBox_KeyDown(IInspectable const &sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const &e);
        void ProfileNameBox_LostFocus(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_ItemInvoked(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                        Microsoft::UI::Xaml::Controls::TreeViewItemInvokedEventArgs const &args);
        void SortByItem_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_ExpandingDeps(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                          Microsoft::UI::Xaml::Controls::TreeViewExpandingEventArgs const &args);
        void SetAsEntryMenuItem_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void DeactivateMenuItem_Click(IInspectable const &sender, Windows::UI::Xaml::RoutedEventArgs const &e);
        void PluginTreeView_Collapsed(Microsoft::UI::Xaml::Controls::TreeView const &sender,
                                      Microsoft::UI::Xaml::Controls::TreeViewCollapsedEventArgs const &args);
        fire_and_forget DeleteMenuItem_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                  winrt::Windows::UI::Xaml::RoutedEventArgs const &e);

      private:
        static inline Windows::Storage::ApplicationData appData{Windows::Storage::ApplicationData::Current()};

        fire_and_forget SaveProfileName();
        void RefreshTreeView();
        void LoadTreeNodesByName();
        void LoadTreeNodesByDependency();
        void LoadTreeNodesByCategory();

        com_ptr<ProfileModel> m_profile{nullptr};
        std::vector<com_ptr<YtFlowApp::implementation::EditPluginModel>> m_pluginModels;
        SortType m_sortType{SortType::ByName};
        event_token m_treeViewExpanding{};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProfilePage : EditProfilePageT<EditProfilePage, implementation::EditProfilePage>
    {
    };
}
