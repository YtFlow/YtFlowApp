#include "pch.h"
#include "EditProfilePage.h"
#if __has_include("EditProfilePage.g.cpp")
#include "EditProfilePage.g.cpp"
#endif

#include "PluginModel.h"
#include "PluginTypeToDescConverter.h"
#include "UI.h"
#include "WinrtScheduler.h"

using namespace std::chrono_literals;
using namespace winrt;
using namespace Windows::UI::Text;
using namespace Windows::UI::Xaml;
using namespace Controls;
using namespace Input;
using namespace Navigation;
namespace muxc = Microsoft::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    EditProfilePage::EditProfilePage()
    {
        try
        {
            VisualStateManager::GoToState(*this, L"MasterState", false);

            auto const weak{get_weak()};
            m_depChangeSubject$.get_observable().debounce(3s, ObserveOnDispatcher()).subscribe([=](bool) {
                try
                {
                    if (auto self{weak.get()})
                    {
                        self->RefreshTreeView();
                    }
                }
                catch (...)
                {
                    NotifyException(L"dep change subscribe");
                }
            });
        }
        catch (...)
        {
            NotifyException(L"EditProfilePage ctor");
        }
    }

    Windows::UI::Text::FontWeight EditProfilePage::PluginNameFontWeight(bool isDirty)
    {
        if (isDirty)
        {
            return Windows::UI::Text::FontWeights::SemiBold();
        }
        return Windows::UI::Text::FontWeights::Normal();
    }

    fire_and_forget EditProfilePage::OnNavigatedTo(NavigationEventArgs const &args)
    {
        try
        {
            const auto lifetime{get_strong()};
            m_profile = args.Parameter().as<ProfileModel>();
            ProfileNameBox().Text(m_profile->Name());
            EditorFrame().Content(nullptr);

            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            const auto plugins{conn.GetPluginsByProfile(m_profile->Id())};
            const auto entryPlugins{conn.GetEntryPluginsByProfile(m_profile->Id())};

            co_await resume_foreground(Dispatcher());
            m_pluginModels.clear();
            m_pluginModels.reserve(plugins.size());
            std::transform(plugins.begin(), plugins.end(), std::back_inserter(m_pluginModels), [&](auto const &p) {
                auto const isEntry{entryPlugins.end() != std::find_if(entryPlugins.begin(), entryPlugins.end(),
                                                                      [&](auto const &ep) { return ep.id == p.id; })};
                return CreateEditPluginModel(p, isEntry);
            });
            RefreshTreeView();
        }
        catch (...)
        {
            NotifyException(L"EditProfilePage OnNavigateTo");
        }
    }
    void EditProfilePage::OnNavigatedFrom(NavigationEventArgs const & /* args */)
    {
        try
        {
            PluginTreeView().RootNodes().Clear();
            m_profile = nullptr;
            m_pluginModels = {};
            if (event_token treeViewExpanding{std::exchange(m_treeViewExpanding, {})})
            {
                PluginTreeView().Expanding(treeViewExpanding);
            }
        }
        catch (...)
        {
            NotifyException(L"EditProfilePage OnNavigatedFrom");
        }
    }
    void EditProfilePage::CheckRenamingPlugin(EditPluginModel *editPluginModel) const &
    {
        auto const name{editPluginModel->Plugin().Name()};
        if (name.size() == 0)
        {
            editPluginModel->HasNamingConflict(true);
            return;
        }
        for (auto const &model : m_pluginModels)
        {
            if (model->Plugin().Name() == name && editPluginModel != model.get())
            {
                editPluginModel->HasNamingConflict(true);
                return;
            }
        }
        editPluginModel->HasNamingConflict(false);
    }
    com_ptr<EditPluginModel> EditProfilePage::CreateEditPluginModel(FfiPlugin const &p, bool isEntry)
    {
        auto const pluginModel{winrt::make_self<PluginModel>(p, m_profile->Id())};
        auto const editPluginModel{winrt::make_self<EditPluginModel>(pluginModel, isEntry)};
        auto const editPluginModelWeak{weak_ref{editPluginModel}};
        auto const weakThis{get_weak()};
        pluginModel->PropertyChanged([=](auto const &, auto const &args) {
            try
            {
                auto const self{weakThis.get()};
                auto const editPluginModel{editPluginModelWeak.get()};
                if (!self || !editPluginModel)
                {
                    return;
                }
                editPluginModel->IsDirty(true);
                auto const propertyName{args.PropertyName()};
                if (propertyName == L"Name" || propertyName == L"Param")
                {
                    m_depChangeSubject$.get_subscriber().on_next(true);
                }
                if (propertyName != L"Name")
                {
                    return;
                }
                self->CheckRenamingPlugin(editPluginModel.get());
            }
            catch (...)
            {
                NotifyException(L"pluginModel PropertyChanged");
            }
        });
        return editPluginModel;
    }
    fire_and_forget EditProfilePage::OnNavigatingFrom(NavigatingCancelEventArgs const &args)
    {
        try
        {
            auto const currState{AdaptiveWidthVisualStateGroup().CurrentState()};
            if (currState != nullptr && currState.Name() == L"DetailState")
            {
                VisualStateManager::GoToState(*this, L"MasterState", true);
                args.Cancel(true);
                co_return;
            }
            auto const navArgs{args};
            auto const lifetime{get_strong()};
            co_await SaveProfileName();

            if (std::exchange(m_forceQuit, false))
            {
                co_return;
            }

            hstring unsavedPluginNames;
            for (auto const &model : m_pluginModels)
            {
                if (model->IsDirty())
                {
                    unsavedPluginNames = unsavedPluginNames + L"\r\n" + model->Plugin().Name();
                }
            }
            if (unsavedPluginNames.empty())
            {
                co_return;
            }

            UnsavedPluginDialogText().Text(std::move(unsavedPluginNames));
            navArgs.Cancel(true);
            if (co_await QuitWithUnsavedDialog().ShowAsync() != ContentDialogResult::Primary)
            {
                co_return;
            }

            m_forceQuit = true;
            switch (navArgs.NavigationMode())
            {
            case NavigationMode::Back:
                Frame().GoBack();
                break;
            case NavigationMode::Forward:
                Frame().GoForward();
                break;
            default:
                Frame().Navigate(navArgs.SourcePageType(), navArgs.NavigationTransitionInfo());
                break;
            }
        }
        catch (...)
        {
            NotifyException(L"EditProfilePage OnNavigatingFrom");
        }
    }

    void EditProfilePage::AdaptiveWidth_StateChanged(IInspectable const & /* sender */,
                                                     VisualStateChangedEventArgs const &e)
    {
        try
        {
            auto const newState{e.NewState()};
            if (newState != nullptr || newState == MediumWidthState())
            {
                return;
            }

            if (EditorFrame().Content() == nullptr)
            {
                VisualStateManager::GoToState(*this, L"MasterState", true);
            }
            else
            {
                VisualStateManager::GoToState(*this, L"DetailState", true);
            }
        }
        catch (...)
        {
            NotifyException(L"EditProfilePage AdaptiveWidth StateChange");
        }
    }

    void EditProfilePage::ProfileNameBox_KeyDown(IInspectable const & /* sender */, KeyRoutedEventArgs const &e)
    {
        try
        {
            if (e.Key() == Windows::System::VirtualKey::Enter)
            {
                SaveProfileName();
            }
        }
        catch (...)
        {
            NotifyException(L"Change profile name");
        }
    }

    void EditProfilePage::ProfileNameBox_LostFocus(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        try
        {
            SaveProfileName();
        }
        catch (...)
        {
            NotifyException(L"Submit profile name");
        }
    }

    IAsyncAction EditProfilePage::SaveProfileName()
    {
        try
        {
            auto const lifetime{get_strong()};
            auto const profile{m_profile};
            auto const newProfileName{ProfileNameBox().Text()};
            if (!profile)
            {
                co_return;
            }
            if (profile->Name() == newProfileName)
            {
                co_return;
            }

            co_await resume_background();

            auto conn{FfiDbInstance.Connect()};
            conn.UpdateProfile(profile->Id(), to_string(newProfileName).data(), to_string(profile->Locale()).data());

            co_await resume_foreground(lifetime->Dispatcher());
            profile->Name(newProfileName);
        }
        catch (...)
        {
            NotifyException(L"Save profile name");
        }
    }

    void EditProfilePage::RefreshTreeView()
    {
        PluginTreeView().RootNodes().Clear();

        if (event_token treeViewExpanding{std::exchange(m_treeViewExpanding, {})})
        {
            PluginTreeView().Expanding(treeViewExpanding);
        }
        SortByNameItem().Icon().Visibility(Visibility::Collapsed);
        SortByDependencyItem().Icon().Visibility(Visibility::Collapsed);
        SortByCategoryItem().Icon().Visibility(Visibility::Collapsed);
        switch (m_sortType)
        {
        case SortType::ByName:
            SortByNameItem().Icon().Visibility(Visibility::Visible);
            LoadTreeNodesByName();
            break;
        case SortType::ByDependency:
            SortByDependencyItem().Icon().Visibility(Visibility::Visible);
            LoadTreeNodesByDependency();
            break;
        case SortType::ByCategory:
            SortByCategoryItem().Icon().Visibility(Visibility::Visible);
            break;
        }
    }

    void EditProfilePage::LoadTreeNodesByName()
    {
        const auto root{PluginTreeView().RootNodes()};
        auto models{m_pluginModels};
        std::sort(models.begin(), models.end(),
                  [](auto const &l, auto const &r) { return l->Plugin().Name() > r->Plugin().Name(); });
        for (auto &model : m_pluginModels)
        {
            muxc::TreeViewNode node{};
            node.Content(YtFlowApp::EditPluginModel(*model));
            root.Append(std::move(node));
        }
    }

    void EditProfilePage::LoadTreeNodesByDependency()
    {
        auto const &tv = PluginTreeView();
        m_treeViewExpanding = tv.Expanding({this, &EditProfilePage::PluginTreeView_ExpandingDeps});
        auto const root{tv.RootNodes()};
        auto models{m_pluginModels};
        std::vector<com_ptr<EditPluginModel>> errorModels;
        std::vector<hstring> usedPlugins;
        size_t idx = 0;

        // Parse root plugins first, including entry plugins and deactivated ones.
        // Entry plugins are displayed and removed.
        while (idx < models.size())
        {
            auto const &model{models[idx]};
            bool hasUnrealizedChildren = false;
            try
            {
                for (auto const &dep : get_self<PluginModel>(model->Plugin())->GetDependencyPlugins())
                {
                    hasUnrealizedChildren = true;
                    usedPlugins.emplace_back(dep);
                }
            }
            catch (FfiException)
            {
                errorModels.emplace_back(model);
                models.erase(models.begin() + idx);
                continue;
            }
            if (model->IsEntry())
            {
                muxc::TreeViewNode node{};
                node.Content(YtFlowApp::EditPluginModel(*model));
                node.HasUnrealizedChildren(hasUnrealizedChildren);
                root.Append(std::move(node));
                models.erase(models.begin() + idx);
            }
            else
            {
                idx++;
            }
        }

        // Remove subplugins, leaving only deactivated root plugins.
        while (usedPlugins.size() > 0)
        {
            auto const desiredPluginName{std::move(usedPlugins[usedPlugins.size() - 1])};
            usedPlugins.pop_back();
            auto const it = std::find_if(models.begin(), models.end(),
                                         [&](auto const &m) { return m->Plugin().Name() == desiredPluginName; });
            if (it == models.end())
            {
                continue;
            }
            auto const model{std::move(*it)};
            models.erase(it);

            try
            {
                for (auto const &dep : get_self<PluginModel>(model->Plugin())->GetDependencyPlugins())
                {
                    usedPlugins.emplace_back(dep);
                }
            }
            catch (FfiException)
            {
                errorModels.emplace_back(model);
                continue;
            }
        }

        if (!models.empty())
        {
            TextBlock tb;
            tb.Text(L"Inactive Plugins");
            muxc::TreeViewNode unusedNode;
            unusedNode.IsExpanded(true);
            unusedNode.Content(std::move(tb));
            for (auto const &model : models)
            {
                muxc::TreeViewNode node;
                node.Content(YtFlowApp::EditPluginModel(*model));
                node.HasUnrealizedChildren(get_self<PluginModel>(model->Plugin())->Verify().required.size() > 0);
                unusedNode.Children().Append(std::move(node));
            }
            root.Append(unusedNode);
        }
        if (!errorModels.empty())
        {
            TextBlock tb;
            tb.Text(L"Error Plugins");
            muxc::TreeViewNode errorNode;
            errorNode.IsExpanded(true);
            errorNode.Content(std::move(tb));
            for (auto const &model : errorModels)
            {
                muxc::TreeViewNode node;
                node.Content(YtFlowApp::EditPluginModel(*model));
                errorNode.Children().Append(std::move(node));
            }
            root.Append(errorNode);
        }
    }

    void EditProfilePage::LoadTreeNodesByCategory()
    {
        // TODO:
    }

    void EditProfilePage::PluginTreeView_ItemInvoked(muxc::TreeView const & /* sender */,
                                                     muxc::TreeViewItemInvokedEventArgs const &args)
    {
        const auto item{args.InvokedItem()};
        const auto tvNode{item.try_as<muxc::TreeViewNode>()};
        if (tvNode == nullptr)
        {
            return;
        }

        const auto editPluginModel{tvNode.Content().try_as<YtFlowApp::EditPluginModel>()};
        if (editPluginModel == nullptr)
        {
            const bool expanded{tvNode.IsExpanded()};
            tvNode.IsExpanded(!expanded);
            return;
        }

        EditorFrame().BackStack().Clear();
        EditorFrame().Navigate(xaml_typename<RawEditorPage>(), editPluginModel,
                               Media::Animation::EntranceNavigationTransitionInfo{});
        auto const currState{AdaptiveWidthVisualStateGroup().CurrentState()};
        if (currState != nullptr && currState.Name() == L"MasterState")
        {
            VisualStateManager::GoToState(*this, L"DetailState", true);
        }
    }

    void EditProfilePage::SortByItem_Click(IInspectable const &sender, RoutedEventArgs const & /* e */)
    {
        try
        {
            const auto item{sender.as<MenuFlyoutItem>()};
            SortType targetSortType;
            if (item == SortByNameItem())
            {
                targetSortType = SortType::ByName;
            }
            else if (item == SortByDependencyItem())
            {
                targetSortType = SortType::ByDependency;
            }
            else if (item == SortByCategoryItem())
            {
                targetSortType = SortType::ByCategory;
            }
            else
            {
                return;
            }
            if (targetSortType == m_sortType)
            {
                return;
            }
            m_sortType = targetSortType;
            appData.LocalSettings().Values().Insert(L"YTFLOW_APP_PROFILE_EDIT_PLUGIN_SORT_TYPE",
                                                    box_value(static_cast<int32_t>(targetSortType)));
            RefreshTreeView();
        }
        catch (...)
        {
            NotifyException(L"Change sorting");
        }
    }

    void EditProfilePage::PluginTreeView_ExpandingDeps(muxc::TreeView const & /* sender */,
                                                       muxc::TreeViewExpandingEventArgs const &args)
    {
        try
        {
            auto const node{args.Node()};
            if (!node.HasUnrealizedChildren())
            {
                return;
            }
            auto const model{args.Item().as<YtFlowApp::EditPluginModel>()};
            try
            {
                node.HasUnrealizedChildren(false);
                for (auto const &dep : get_self<PluginModel>(model.Plugin())->GetDependencyPlugins())
                {
                    auto const it = std::find_if(m_pluginModels.begin(), m_pluginModels.end(),
                                                 [&](auto const &m) { return m->Plugin().Name() == dep; });
                    if (it == m_pluginModels.end())
                    {
                        continue;
                    }
                    muxc::TreeViewNode subNode;
                    subNode.Content(YtFlowApp::EditPluginModel(**it));
                    try
                    {
                        subNode.HasUnrealizedChildren(get_self<PluginModel>((*it)->Plugin())->Verify().required.size() >
                                                      0);
                    }
                    catch (FfiException)
                    {
                        // Do nothing. Treat a plugin with errors as a 'dead end'.
                    }
                    node.Children().Append(std::move(subNode));
                }
            }
            catch (FfiException)
            {
                node.IsExpanded(false);
            }
        }
        catch (...)
        {
            NotifyException(L"Expanding");
        }
    }

    fire_and_forget EditProfilePage::SetAsEntryMenuItem_Click(IInspectable const &sender,
                                                              RoutedEventArgs const & /* e */)
    {
        try
        {
            auto const lifetime{get_strong()};
            auto const model{sender.as<FrameworkElement>()
                                 .DataContext()
                                 .as<muxc::TreeViewNode>()
                                 .Content()
                                 .as<YtFlowApp::EditPluginModel>()};
            co_await resume_background();
            get_self<PluginModel>(model.Plugin())->SetAsEntry();
            co_await resume_foreground(Dispatcher());
            model.IsEntry(true);
            if (m_sortType == SortType::ByDependency)
            {
                RefreshTreeView();
            }
        }
        catch (...)
        {
            NotifyException(L"Set as entry");
        }
    }

    fire_and_forget EditProfilePage::DeactivateMenuItem_Click(IInspectable const &sender,
                                                              RoutedEventArgs const & /* e */)
    {
        try
        {
            auto const lifetime{get_strong()};
            auto const model{sender.as<FrameworkElement>()
                                 .DataContext()
                                 .as<muxc::TreeViewNode>()
                                 .Content()
                                 .as<YtFlowApp::EditPluginModel>()};
            co_await resume_background();
            get_self<PluginModel>(model.Plugin())->UnsetAsEntry();
            co_await resume_foreground(Dispatcher());
            model.IsEntry(false);
            if (m_sortType == SortType::ByDependency)
            {
                RefreshTreeView();
            }
        }
        catch (...)
        {
            NotifyException(L"Deactivate");
        }
    }

    void EditProfilePage::PluginTreeView_Collapsed(muxc::TreeView const & /* sender */,
                                                   muxc::TreeViewCollapsedEventArgs const &args)
    {
        try
        {
            auto const model{args.Item().try_as<YtFlowApp::EditPluginModel>()};
            if (model == nullptr)
            {
                return;
            }

            auto const node{args.Node()};
            node.Children().Clear();
            node.HasUnrealizedChildren(true);
        }
        catch (...)
        {
            NotifyException(L"Collapsed");
        }
    }

    fire_and_forget EditProfilePage::DeleteMenuItem_Click(IInspectable const &sender, RoutedEventArgs const &e)
    {
        static bool deleting = false;
        if (deleting)
        {
            co_return;
        }
        deleting = true;
        try
        {
            auto const lifetime{get_strong()};
            auto const model{sender.as<FrameworkElement>()
                                 .DataContext()
                                 .as<muxc::TreeViewNode>()
                                 .Content()
                                 .as<YtFlowApp::EditPluginModel>()};
            ConfirmPluginDeleteDialog().Content(model);
            auto const ret{co_await ConfirmPluginDeleteDialog().ShowAsync()};
            if (ret != ContentDialogResult::Primary)
            {
                deleting = false;
                co_return;
            }
            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            conn.DeletePlugin(model.Plugin().Id());
            co_await resume_foreground(Dispatcher());
            deleting = false;
            ConfirmPluginDeleteDialog().Content(nullptr);
            com_ptr<EditPluginModel> modelPtr;
            modelPtr.copy_from(get_self<EditPluginModel>(model));
            auto const it{std::find(m_pluginModels.begin(), m_pluginModels.end(), modelPtr)};
            if (it == m_pluginModels.end())
            {
                co_return;
            }
            m_pluginModels.erase(it);
            RefreshTreeView();
        }
        catch (...)
        {
            NotifyException(L"Deleting");
        }
    }

    fire_and_forget EditProfilePage::AddPluginButton_Click(IInspectable const & /* sender */,
                                                           RoutedEventArgs const & /* e */)
    {
        try
        {
            auto const lifetime{get_strong()};
            auto const res{co_await AddPluginDialog().ShowAsync()};
            if (res != ContentDialogResult::Primary)
            {
                co_return;
            };

            auto const pluginType{NewPluginTypeBox().SelectedValue().as<hstring>()};
            auto const pluginName{NewPluginNameText().Text()};
            FfiPlugin ffiPlugin;
            ffiPlugin.name = to_string(pluginName);
            ffiPlugin.desc = to_string(PluginTypeToDescConverter::DescMap.find(pluginType)->second.as<hstring>());
            ffiPlugin.plugin = to_string(pluginType);
            ffiPlugin.plugin_version = 0;
            ffiPlugin.param = std::vector<uint8_t>{0xf6}; // TODO: param?

            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            ffiPlugin.id = conn.CreatePlugin(m_profile->Id(), ffiPlugin.name.data(), ffiPlugin.desc.data(),
                                             ffiPlugin.plugin.data(), ffiPlugin.plugin_version, ffiPlugin.param.data(),
                                             ffiPlugin.param.size());
            co_await resume_foreground(Dispatcher());

            for (auto const &model : m_pluginModels)
            {
                if (model->Plugin().Name() == pluginName)
                {
                    model->HasNamingConflict(true);
                }
            }

            auto editPluginModel{CreateEditPluginModel(ffiPlugin, false)};
            m_pluginModels.push_back(std::move(editPluginModel));

            RefreshTreeView();
        }
        catch (...)
        {
            NotifyException(L"Adding");
        }
    }

    void EditProfilePage::NewPluginNameText_TextChanged(IInspectable const & /* sender */,
                                                        TextChangedEventArgs const & /* e */)
    {
        NewPluginNameText().Foreground({nullptr});
    }

    void EditProfilePage::AddPluginDialog_Closing(ContentDialog const & /* sender */,
                                                  ContentDialogClosingEventArgs const &args)
    {
        try
        {
            if (args.Result() != ContentDialogResult::Primary)
            {
                return;
            }
            auto const pluginName{NewPluginNameText().Text()};
            if (pluginName == L"")
            {
                NewPluginNameText().Focus(FocusState::Programmatic);
                args.Cancel(true);
                return;
            }
            auto const pluginNameStr = to_string(pluginName);
            auto const it{std::find_if(m_pluginModels.begin(), m_pluginModels.end(), [&](auto const &model) {
                return get_self<PluginModel>(model->Plugin())->OriginalPlugin.name == pluginNameStr;
            })};
            if (it != m_pluginModels.end())
            {
                NewPluginNameText().Foreground(Media::SolidColorBrush{Windows::UI::Colors::Red()});
                args.Cancel(true);
            }
        }
        catch (...)
        {
            NotifyException(L"Validating new plugin");
        }
    }
}
