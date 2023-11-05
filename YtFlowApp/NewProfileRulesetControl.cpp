#include "pch.h"
#include "NewProfileRulesetControl.h"
#if __has_include("NewProfileRulesetControl.g.cpp")
#include "NewProfileRulesetControl.g.cpp"
#endif

#include "winrt\Windows.Web.Http.Filters.h"
#include "winrt\Windows.Web.Http.Headers.h"

#include "UI.h"
#include "VectorBuffer.h"

using namespace winrt;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Controls;
using namespace Windows::Storage;
using namespace Streams;
using namespace Windows::Web::Http;
using namespace Filters;

namespace winrt::YtFlowApp::implementation
{
    bool NewProfileRulesetControl::RulesetSelected()
    {
        return m_rulesetSelected;
    }
    hstring NewProfileRulesetControl::RulesetName()
    {
        return m_rulesetName;
    }

    std::string_view NewProfileRulesetControl::GetResourceKeyFromSelectedRuleset()
    {
        auto const selectedItemControl = SelectionComboBox().SelectedItem().try_as<ComboBoxItem>();
        if (selectedItemControl == nullptr)
        {
            return "";
        }

        auto const tag = selectedItemControl.Tag().as<hstring>();
        if (tag == L"dreamacro-geoip")
        {
            return "dreamacro-geoip";
        }
        if (tag == L"loyalsoldier-country-only-cn-private")
        {
            return "loyalsoldier-country-only-cn-private";
        }
        if (tag == L"loyalsoldier-surge-proxy")
        {
            return "loyalsoldier-surge-proxy";
        }
        if (tag == L"loyalsoldier-surge-direct")
        {
            return "loyalsoldier-surge-direct";
        }
        if (tag == L"loyalsoldier-surge-private")
        {
            return "loyalsoldier-surge-private";
        }
        if (tag == L"loyalsoldier-surge-reject")
        {
            return "loyalsoldier-surge-reject";
        }
        if (tag == L"loyalsoldier-surge-tld-not-cn")
        {
            return "loyalsoldier-surge-tld-not-cn";
        }
        return "";
    }

    IAsyncOperation<bool> NewProfileRulesetControl::BatchUpdateRulesetsIfNotExistAsync(std::vector<hstring> rulesetKeys)
    {
        auto const lifetime = get_strong();
        for (auto const &rulesetKey : rulesetKeys)
        {
            auto const items = SelectionComboBox().Items();
            auto const item = std::ranges::find_if(
                items, [&](auto const it) { return it.as<ComboBoxItem>().Tag().as<hstring>() == rulesetKey; });
            if (item == items.end())
            {
                continue;
            }

            SelectionComboBox().SelectedItem(*item);
            if (!co_await InitSelectedRuleset())
            {
                co_return false;
            }
            if (IsPrimaryButtonEnabled())
            {
                continue;
            }

            if (!co_await UpdateAsync())
            {
                co_return false;
            }
        }
        co_return true;
    }

    IAsyncOperation<bool> NewProfileRulesetControl::InitSelectedRuleset()
    {
        auto const lifetime = get_strong();
        std::exception_ptr ex{nullptr};
        try
        {
            auto const resourceKey = lifetime->GetResourceKeyFromSelectedRuleset();
            if (resourceKey == "")
            {
                co_return false;
            }

            lifetime->SelectionComboBox().IsEnabled(false);
            lifetime->UpdateButton().IsEnabled(false);
            lifetime->UpdateErrorText().Text(L"");
            lifetime->IsPrimaryButtonEnabled(false);

            auto const shouldLoadResources = lifetime->m_resources.empty();
            std::optional<FfiResource> resource = std::nullopt;
            if (!shouldLoadResources)
            {
                auto const resourceIt =
                    std::ranges::find_if(lifetime->m_resources, [&](auto const &r) { return r.key == resourceKey; });
                if (resourceIt != lifetime->m_resources.end())
                {
                    resource = *resourceIt;
                }
            }

            StorageFolder folder = lifetime->m_resourceFolder;
            co_await resume_background();

            if (folder == nullptr)
            {
                folder = co_await ApplicationData::Current().LocalFolder().CreateFolderAsync(
                    L"resource", CreationCollisionOption::OpenIfExists);
            }

            std::vector<FfiResource> resources;
            auto conn = FfiDbInstance.Connect();
            if (shouldLoadResources)
            {
                resources = conn.GetResources();
                auto const resourceIt =
                    std::ranges::find_if(resources, [&](auto const &r) { return r.key == resourceKey; });
                if (resourceIt != resources.end())
                {
                    resource = *resourceIt;
                }
            }
            hstring lastUpdated;
            bool fileExists = false;
            if (resource.has_value())
            {
                if (resource->remote_type == "url")
                {
                    auto const resourceUrl = conn.GetResourceUrlByResourceId(resource->id);
                    lastUpdated = to_hstring(resourceUrl.retrieved_at.value_or("never"));
                }
                else if (resource->remote_type == "github_release")
                {
                    auto const resourceUrl = conn.GetResourceGitHubReleaseByResourceId(resource->id);
                    lastUpdated = to_hstring(resourceUrl.retrieved_at.value_or("never"));
                }
                else
                {
                    lastUpdated = L"unknown";
                }
                if (!resource->local_file.empty())
                {
                    auto const item = co_await folder.TryGetItemAsync(to_hstring(resource->local_file));
                    fileExists = item != nullptr && item.IsOfType(StorageItemTypes::File);
                }
            }
            else
            {
                lastUpdated = L"never";
            }

            co_await resume_foreground(lifetime->Dispatcher());
            if (shouldLoadResources && lifetime->m_resources.empty())
            {
                lifetime->m_resources = std::move(resources);
            }
            if (lifetime->m_resourceFolder == nullptr)
            {
                lifetime->m_resourceFolder = std::move(folder);
            }
            lifetime->IsPrimaryButtonEnabled(fileExists);
            lifetime->LastUpdatedText().Text(std::move(lastUpdated));
        }
        catch (...)
        {
            ex = std::current_exception();
        }
        if (ex != nullptr)
        {
            co_await resume_foreground(lifetime->Dispatcher());
            lifetime->Hide();
            try
            {
                std::rethrow_exception(ex);
            }
            catch (...)
            {
                NotifyException(L"Initializing resource");
            }
        }
        lifetime->SelectionComboBox().IsEnabled(true);
        lifetime->UpdateButton().IsEnabled(true);
        co_return ex == nullptr;
    }

    fire_and_forget NewProfileRulesetControl::SelectionComboBox_SelectionChanged(IInspectable const &,
                                                                                 SelectionChangedEventArgs const &)
    {
        co_await InitSelectedRuleset();
    }

    fire_and_forget NewProfileRulesetControl::ContentDialog_Opened(ContentDialog const &,
                                                                   ContentDialogOpenedEventArgs const &)
    {
        m_rulesetSelected = false;
        m_selectionChangeToken =
            SelectionComboBox().SelectionChanged({this, &NewProfileRulesetControl::SelectionComboBox_SelectionChanged});
        co_await InitSelectedRuleset();
    }

    void NewProfileRulesetControl::ContentDialog_Closing(ContentDialog const &,
                                                         ContentDialogClosingEventArgs const &args)
    {
        if (m_updating)
        {
            args.Cancel(true);
        }
        else
        {
            if (auto const selectionChangeToken{std::exchange(m_selectionChangeToken, {})})
            {
                SelectionComboBox().SelectionChanged(selectionChangeToken);
            }
        }
    }

    void NewProfileRulesetControl::CancelUpdateButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        if (m_updating)
        {
            m_updateCancelled.store(true);
        }
    }

    fire_and_forget NewProfileRulesetControl::UpdateButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        co_await UpdateAsync();
    }

    IAsyncOperation<bool> NewProfileRulesetControl::UpdateAsync()
    {
        auto const lifetime = get_strong();
        std::exception_ptr ex{nullptr};
        StorageFile file{nullptr};
        auto transferFinished = std::make_shared<bool>();

        if (m_client == nullptr)
        {
            HttpBaseProtocolFilter const clientFilter;
            clientFilter.CacheControl().ReadBehavior(HttpCacheReadBehavior::MostRecent);
            m_client = HttpClient(clientFilter);
        }
        auto const client = m_client;
        bool finishedUpdate{};
        try
        {
            lifetime->m_updateCancelled.store(false);
            lifetime->IsPrimaryButtonEnabled(false);
            lifetime->SelectionComboBox().IsEnabled(false);
            lifetime->UpdateButton().IsEnabled(false);
            lifetime->CancelUpdateButton().Visibility(Visibility::Visible);
            lifetime->UpdateProgressBar().Value(.0f);

            auto const resourceKey = lifetime->GetResourceKeyFromSelectedRuleset();
            std::optional<FfiResource> resource = std::nullopt;
            auto const resourceIt =
                std::ranges::find_if(lifetime->m_resources, [&](auto const &r) { return r.key == resourceKey; });
            if (resourceIt != lifetime->m_resources.end())
            {
                resource = *resourceIt;
            }

            co_await resume_background();

            UUID newUuid{};
            UuidCreateSequential(&newUuid);
            std::array<char, 37> newUuidBuf{};
            if (sprintf_s(newUuidBuf.data(), sizeof(newUuidBuf), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                          newUuid.Data1, newUuid.Data2, newUuid.Data3, newUuid.Data4[0], newUuid.Data4[1],
                          newUuid.Data4[2], newUuid.Data4[3], newUuid.Data4[4], newUuid.Data4[5], newUuid.Data4[6],
                          newUuid.Data4[7]) < 0)
            {
                throw std::invalid_argument("Cannot format UUID");
            };
            auto const newFileName = std::string(newUuidBuf.data());
            auto const newFileNameW = to_hstring(newFileName);

            std::string url, oldFileName;
            uint32_t resourceId;
            if (resource.has_value())
            {
                if (resource->remote_type != "url")
                {
                    throw std::invalid_argument("Unknown remote type");
                }
                auto conn = FfiDbInstance.Connect();
                auto resourceUrl = conn.GetResourceUrlByResourceId(resource->id);
                resourceId = resource->id;
                url = std::move(resourceUrl.url);
                oldFileName = resource->local_file;
                // TODO: etag, last_modified
            }
            else
            {
                char const *type;
                if (resourceKey == "dreamacro-geoip")
                {
                    type = "geoip-country";
                    url = "https://cdn.jsdelivr.net/gh/Dreamacro/maxmind-geoip@release/Country.mmdb";
                }
                else if (resourceKey == "loyalsoldier-country-only-cn-private")
                {
                    type = "geoip-country";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/geoip@release/Country-only-cn-private.mmdb";
                }
                else if (resourceKey == "loyalsoldier-surge-proxy")
                {
                    type = "surge-domain-set";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/surge-rules@release/proxy.txt";
                }
                else if (resourceKey == "loyalsoldier-surge-direct")
                {
                    type = "surge-domain-set";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/surge-rules@release/direct.txt";
                }
                else if (resourceKey == "loyalsoldier-surge-private")
                {
                    type = "surge-domain-set";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/surge-rules@release/private.txt";
                }
                else if (resourceKey == "loyalsoldier-surge-reject")
                {
                    type = "surge-domain-set";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/surge-rules@release/reject.txt";
                }
                else if (resourceKey == "loyalsoldier-surge-tld-not-cn")
                {
                    type = "surge-domain-set";
                    url = "https://cdn.jsdelivr.net/gh/Loyalsoldier/surge-rules@release/tld-not-cn.txt";
                }
                else
                {
                    throw std::invalid_argument("Unknown resource key for URL");
                }
                auto conn = FfiDbInstance.Connect();
                resourceId = conn.CreateResourceWithUrl(std::string(resourceKey).c_str(), type, newFileName.c_str(),
                                                        url.c_str());
                oldFileName = newFileName;
            }

            hstring errMsg{};
            try
            {
                client.DefaultRequestHeaders().UserAgent().Clear();
                client.DefaultRequestHeaders().UserAgent().ParseAdd(L"YtFlowApp/0.0 ResourceUpdater/0.0");
                HttpRequestMessage req;
                req.RequestUri(Uri{to_hstring(url)});
                auto const res =
                    co_await client.SendRequestAsync(std::move(req), HttpCompletionOption::ResponseHeadersRead);
                auto const _ = res.EnsureSuccessStatusCode();
                auto const headers = res.Headers();
                auto const newEtag = headers.TryLookup(L"etag");
                auto const newLastModified = headers.TryLookup(L"last-modified");
                auto const content = res.Content();
                uint64_t resLen = std::optional<uint64_t>(content.Headers().ContentLength()).value_or(0);
                file = co_await ApplicationData::Current().TemporaryFolder().CreateFileAsync(newFileNameW);
                auto const fstream = co_await file.OpenAsync(FileAccessMode::ReadWrite);

                co_await resume_foreground(lifetime->Dispatcher());
                lifetime->UpdateProgressBar().Visibility(Visibility::Visible);
                lifetime->UpdateProgressBar().IsIndeterminate(resLen == 0);
                lifetime->m_updating = true;
                co_await resume_background(); // Also a hack that unblocks the stream copy operation below.

                auto const resStream = co_await content.ReadAsInputStreamAsync();
                // Hand written stream copy as RandomAccessStream::CopyAsync does not support cancellation.
                auto const buf = make<VectorBuffer>(std::vector<uint8_t>(32768));
                auto totalWritten = std::make_shared<std::atomic_uint64_t>();
                [](uint64_t const resLen, com_ptr<NewProfileRulesetControl> lifetime,
                   std::shared_ptr<std::atomic_uint64_t> totalWritten,
                   std::shared_ptr<bool> transferFinished) -> fire_and_forget {
                    if (resLen == 0)
                    {
                        co_return;
                    }
                    while (!*transferFinished)
                    {
                        co_await std::chrono::seconds{1};
                        auto const percentage =
                            static_cast<double>(totalWritten->load()) / static_cast<double>(resLen) * 100.0;
                        co_await resume_foreground(lifetime->Dispatcher());
                        lifetime->UpdateProgressBar().Value(percentage);
                    }
                }(resLen, lifetime, totalWritten, transferFinished);
                while (true)
                {
                    buf.Length(0);
                    co_await resume_foreground(lifetime->Dispatcher());
                    if (lifetime->m_updateCancelled.load())
                    {
                        throw hresult_canceled{};
                    }
                    auto const readBuf = co_await resStream.ReadAsync(buf, buf.Capacity(), InputStreamOptions::Partial);

                    auto toWrite = readBuf.Length();
                    if (toWrite == 0)
                    {
                        break;
                    }

                    while (toWrite > 0)
                    {
                        if (lifetime->m_updateCancelled.load())
                        {
                            throw hresult_canceled{};
                        }
                        auto const written = co_await fstream.WriteAsync(readBuf);
                        toWrite -= written;
                        totalWritten->fetch_add(written);
                        if (memcpy_s(readBuf.data(), readBuf.Capacity(), readBuf.data() + written, toWrite) != 0)
                        {
                            throw std::invalid_argument("Failed to move read data");
                        }
                    }
                }
                co_await fstream.FlushAsync();
                fstream.Close();
                content.Close();

                co_await file.MoveAsync(lifetime->m_resourceFolder, to_hstring(oldFileName),
                                        NameCollisionOption::ReplaceExisting);
                file = nullptr;

                co_await resume_background();
                auto conn = FfiDbInstance.Connect();
                conn.UpdateResourceUrlRetrievedByResourceId(
                    resourceId, newEtag.has_value() ? to_string(*newEtag).c_str() : nullptr,
                    newLastModified.has_value() ? to_string(*newLastModified).c_str() : nullptr);
                finishedUpdate = true;
            }
            catch (hresult_canceled const &)
            {
            }
            catch (hresult_error const &hr)
            {
                errMsg = hr.message();
            }

            co_await resume_foreground(lifetime->Dispatcher());

            lifetime->m_resources.clear();
            lifetime->m_updating = false;
            (void)lifetime->InitSelectedRuleset();
            lifetime->UpdateErrorText().Text(std::move(errMsg));
        }
        catch (...)
        {
            ex = std::current_exception();
        }
        try
        {
            if (file != nullptr)
            {
                co_await file.DeleteAsync();
            }
        }
        catch (...)
        {
            // Delete the temp file at best effort
        }
        if (ex != nullptr)
        {
            co_await resume_foreground(lifetime->Dispatcher());
            lifetime->m_updating = false; // Allow the dialog to Close
            lifetime->Hide();
            try
            {
                std::rethrow_exception(ex);
            }
            catch (...)
            {
                NotifyException(L"Updating");
            }
            (void)lifetime->InitSelectedRuleset();
        }
        *transferFinished = true;
        lifetime->UpdateButton().IsEnabled(true);
        lifetime->CancelUpdateButton().Visibility(Visibility::Collapsed);
        lifetime->UpdateProgressBar().Visibility(Visibility::Collapsed);
        co_return (ex == nullptr) && finishedUpdate;
    }
    void NewProfileRulesetControl::ContentDialog_PrimaryButtonClick(ContentDialog const &,
                                                                    ContentDialogButtonClickEventArgs const &)
    {
        m_rulesetSelected = true;
        m_rulesetName = to_hstring(GetResourceKeyFromSelectedRuleset());
    }
}
