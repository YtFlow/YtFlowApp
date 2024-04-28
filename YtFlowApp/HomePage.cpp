#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

#include <winrt/Windows.Storage.Pickers.h>

using namespace concurrency;

#include "ConnectionState.h"
#include "CoreFfi.h"
#include "EditProfilePage.h"
#include "HomeProfileControl.h"
#include "ProfileModel.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;

namespace winrt::YtFlowApp::implementation
{
    HomePage::HomePage()
    {
        InitializeComponent();
    }

    void HomePage::OnNavigatedFrom(Navigation::NavigationEventArgs const & /* args */)
    {
        m_connStatusChangeSubscription$.unsubscribe();
        m_refreshPluginStatus$.unsubscribe();
        PluginWidgetPanel().Children().Clear();
        ConnectedViewSidePanel().Children().Clear();
        m_widgets.clear();
    }

    IAsyncAction HomePage::EnsureDatabase()
    {
        if (FfiDbInstance != nullptr)
        {
            co_return;
        }

        const auto localFolder = appData.LocalFolder();
        const auto dbFolder =
            co_await localFolder.CreateFolderAsync(L"db", Windows::Storage::CreationCollisionOption::OpenIfExists);
        hstring const dbPath = dbFolder.Path() + L"\\main.db";
        appData.LocalSettings().Values().Insert(L"YTFLOW_DB_PATH", box_value(dbPath));

        FfiDbInstance = FfiDb::Open(dbPath);
    }

    HomePage::RequestSender HomePage::MakeRequestSender(uint32_t pluginId)
    {
        auto weak = get_weak();
        return [weak = std::move(weak), pluginId](std::string_view func, std::vector<uint8_t> param) {
            return [](auto weak, auto id, auto func, auto param) -> task<std::vector<uint8_t>> {
                auto self{weak.get()};
                // TODO: self is nullptr
                auto const ret{co_await self->m_rpc.load()->SendRequestToPlugin(id, func, std::move(param))};
                self->m_triggerInfoUpdate$.get_subscriber().on_next(true);
                co_return ret;
            }(weak, pluginId, func, param);
        };
    }

    Collections::IObservableVector<YtFlowApp::ProfileModel> HomePage::Profiles() const
    {
        return m_profiles;
    }

    void HomePage::OnConnectRequested(Windows::Foundation::IInspectable const & /* sender */,
                                      YtFlowApp::HomeProfileControl const &control)
    {
        try
        {
            if (m_vpnTask != nullptr)
            {
                m_vpnTask.Cancel();
            }
            m_vpnTask =
                connectToProfile(get_self<ProfileModel>(get_self<HomeProfileControl>(control)->Profile())->Id());
        }
        catch (...)
        {
            NotifyException(L"Connect request");
        }
    }
    void HomePage::OnEditRequested(Windows::Foundation::IInspectable const & /* sender */,
                                   YtFlowApp::HomeProfileControl const &control)
    {
        Frame().Navigate(xaml_typename<YtFlowApp::EditProfilePage>(), get_self<HomeProfileControl>(control)->Profile(),
                         Media::Animation::DrillInNavigationTransitionInfo{});
    }
    fire_and_forget HomePage::OnExportRequested(IInspectable const &, YtFlowApp::HomeProfileControl const &control)
    {
        try
        {
            auto const lifetime{get_strong()};

            auto const profile = get_self<ProfileModel>(get_self<HomeProfileControl>(control)->Profile());
            static Windows::Storage::Pickers::FileSavePicker picker = nullptr;
            if (picker == nullptr)
            {
                picker = Windows::Storage::Pickers::FileSavePicker();
                picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::Downloads);
                picker.FileTypeChoices().Insert(L"YtFlow TOML Profile", single_threaded_vector<hstring>({L".toml"}));
            }
            picker.SuggestedFileName(profile->Name() + L".ytp");
            auto const file = co_await picker.PickSaveFileAsync();
            if (file == nullptr)
            {
                co_return;
            }
            auto const profileId = profile->Id();

            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            auto const data{conn.ExportProfileToml(profileId)};
            Windows::Storage::CachedFileManager::DeferUpdates(file);
            co_await Windows::Storage::FileIO::WriteBytesAsync(
                file, array_view<uint8_t const>(reinterpret_cast<uint8_t const *>(data.data()),
                                                static_cast<uint32_t>(data.size())));
            co_await Windows::Storage::CachedFileManager::CompleteUpdatesAsync(file); // TODO: sync result

            co_await resume_foreground(lifetime->Dispatcher());
            NotifyUser(L"Profile exported successfully. Make sure sensitive information inside is removed before "
                       L"sharing this profile.",
                       L"Export Profile");
        }
        catch (...)
        {
            NotifyException(L"Exporting profile");
        }
    }
    fire_and_forget HomePage::OnDeleteRequested(Windows::Foundation::IInspectable const & /* sender */,
                                                YtFlowApp::HomeProfileControl const &control)
    {
        static bool deleting = false;
        try
        {
            if (std::exchange(deleting, true))
            {
                co_return;
            }
            auto const lifetime{get_strong()};
            auto const profile = get_self<HomeProfileControl>(control)->Profile();
            ConfirmProfileDeleteDialog().Content(profile);
            if (std::exchange(isDialogShown, true))
            {
                deleting = false;
                co_return;
            }
            auto const ret{co_await ConfirmProfileDeleteDialog().ShowAsync()};
            isDialogShown = false;
            if (ret != ContentDialogResult::Primary)
            {
                deleting = false;
                co_return;
            }
            co_await resume_background();
            auto conn{FfiDbInstance.Connect()};
            conn.DeleteProfile(profile.Id());
            co_await resume_foreground(Dispatcher());
            deleting = false;
            ConfirmProfileDeleteDialog().Content(nullptr);
            uint32_t index;
            if (!m_profiles.IndexOf(profile, index))
            {
                co_return;
            }
            m_profiles.RemoveAt(index);
        }
        catch (...)
        {
            NotifyException(L"Deleting profile");
        }
        co_await resume_foreground(Dispatcher());
        deleting = false;
    }
    IAsyncAction HomePage::connectToProfile(uint32_t id)
    {
        auto const localSettings = appData.LocalSettings().Values();
        localSettings.Insert(L"YTFLOW_PROFILE_ID", box_value(id));
        auto connectTask{ConnectionState::Instance->Connect()};
        auto const cancelToken{co_await get_cancellation_token()};
        cancelToken.callback([=]() {
            connectTask.Cancel();
            localSettings.TryRemove(YTFLOW_CORE_ERROR_LOAD);
        });
        auto ret = co_await connectTask;
        if (ret == VpnManagementErrorStatus::Other)
        {
            auto coreError = localSettings.TryLookup(YTFLOW_CORE_ERROR_LOAD).try_as<hstring>();
            if (!coreError.has_value())
            {
                NotifyUser(hstring{std::format(L"Failed to connect VPN. Please connect YtFlow Auto using system "
                                               L"settings for detailed error messages..")},
                           L"VPN Error");
            }
        }
        else if (ret != VpnManagementErrorStatus::Ok)
        {
            NotifyUser(hstring{std::format(L"Failed to connect VPN: error code {}", static_cast<int32_t>(ret))},
                       L"VPN Error");
        }
    }

    void HomePage::ConnectCancelButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
            m_vpnTask = nullptr;
        }
    }

    void HomePage::DisconnectButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        if (m_vpnTask != nullptr)
        {
            m_vpnTask.Cancel();
        }
        m_vpnTask = []() -> IAsyncAction { co_await ConnectionState::Instance->Disconnect(); }();
    }
    void HomePage::CreateProfileButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        Frame().Navigate(xaml_typename<NewProfilePage>());
    }
    fire_and_forget HomePage::ImportProfileButton_Click(Windows::Foundation::IInspectable const &sender,
                                                        Windows::UI::Xaml::RoutedEventArgs const &e)
    {
        auto const lifetime{get_strong()};
        try
        {
            static Windows::Storage::Pickers::FileOpenPicker picker = nullptr;
            if (picker == nullptr)
            {
                picker = Windows::Storage::Pickers::FileOpenPicker();
                picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::Downloads);
                picker.FileTypeFilter().Append(L".toml");
            }
            auto const file = co_await picker.PickSingleFileAsync();
            if (file == nullptr)
            {
                co_return;
            }
            auto const data = co_await Windows::Storage::FileIO::ReadBufferAsync(file);

            co_await resume_background();
            FfiParsedTomlProfile profile;
            {
                auto conn{FfiDbInstance.Connect()};
                profile = conn.ParseProfileToml(data.data(), data.Length());
            }

            co_await resume_foreground(lifetime->Dispatcher());
            if (std::exchange(isDialogShown, true))
            {
                co_return;
            }
            ConfirmProfileImportDialogPluginCountText().Text(to_hstring(profile.plugins.size()));
            ConfirmProfileImportDialogProfileNameText().Text(to_hstring(profile.name.value_or("Unnamed Profile")));
            auto const ret = co_await ConfirmProfileImportDialog().ShowAsync();
            isDialogShown = false;
            if (ret != ContentDialogResult::Primary)
            {
                co_return;
            }

            co_await resume_background();
            auto conn = FfiDbInstance.Connect();
            auto const existingProfiles = conn.GetProfiles();
            auto const baseProfileName = std::move(profile.name).value_or("Unnamed Profile");
            auto newProfileName = baseProfileName;
            auto newProfileSuffix = 0;

            using std::ranges::find_if;
            while (find_if(existingProfiles, [&](auto const &p) { return p.name == newProfileName; }) !=
                   existingProfiles.end())
            {
                newProfileName = baseProfileName + " " + std::to_string(++newProfileSuffix);
            }

            auto locale = profile.locale.value_or("en-US");
            auto const newProfileId = conn.CreateProfile(newProfileName.c_str(), locale.c_str());
            for (auto &&plugin : profile.plugins)
            {
                auto const pluginId = conn.CreatePlugin(
                    newProfileId, plugin.plugin.name.c_str(), plugin.plugin.desc.c_str(), plugin.plugin.plugin.c_str(),
                    plugin.plugin.plugin_version, plugin.plugin.param.data(), plugin.plugin.param.size());
                if (plugin.is_entry)
                {
                    conn.SetPluginAsEntry(pluginId, newProfileId);
                }
            }

            co_await resume_foreground(lifetime->Dispatcher());
            m_profiles.Append(make<ProfileModel>(FfiProfile{
                .id = newProfileId,
                .name = std::move(newProfileName),
                .locale = std::move(locale),
            }));
        }
        catch (...)
        {
            NotifyException(L"Importing profile");
        }
    }
}
