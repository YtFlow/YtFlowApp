#include "pch.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    std::map<uint32_t, std::string_view> errorFmtStrings = {{1, ""}};
    const char *FfiException::what() const throw()
    {
        return msg.c_str();
    }
    FfiException::FfiException(ytflow_core::ytflow_result const &r)
    {
        // TODO: all errors
        if (r.data.err[0] == nullptr)
        {
            msg = std::format("Unknown error {}", r.code);
        }
        else if (r.data.err[1] == nullptr)
        {
            msg = std::format("Unknown error {}({})", r.code, r.data.err[0]);
        }
        else if (r.data.err[2] == nullptr)
        {
            msg = std::format("Unknown error {}({}, {})", r.code, r.data.err[0], r.data.err[1]);
        }
        else
        {
            msg = std::format("Unknown error {}({}, {}, {})", r.code, r.data.err[0], r.data.err[1], r.data.err[2]);
        }
    }

    FfiPluginVerifyResult FfiPlugin::verify(char const *plugin, uint16_t plugin_version, uint8_t const *param,
                                            size_t param_len)
    {
        return unwrap_ffi_buffer<FfiPluginVerifyResult>(
            ytflow_core::ytflow_plugin_verify(plugin, plugin_version, param, param_len));
    }

    FfiDb::FfiDb(FfiDb &&that) noexcept : db_ptr(that.db_ptr)
    {
        that.db_ptr = nullptr;
    }
    FfiDb &FfiDb::operator=(FfiDb &&that)
    {
        if (db_ptr != nullptr)
        {
            unwrap_ffi_result<FfiNoop>(ytflow_db_free(db_ptr));
        }
        db_ptr = that.db_ptr;
        that.db_ptr = nullptr;
        return *this;
    }
    bool FfiDb::operator==(std::nullptr_t) const noexcept
    {
        return db_ptr == nullptr;
    }
    FfiDb FfiDb::Open(std::wstring_view path)
    {
        auto result = ytflow_core::ytflow_db_new_win32(reinterpret_cast<const uint16_t *>(path.data()), path.size());
        return unwrap_ffi_result<FfiDb>(result);
    }

    FfiDb FfiDb::from_ffi(void *ptr1, uintptr_t)
    {
        return FfiDb(static_cast<ytflow_core::ytflow_database *>(ptr1));
    }
    FfiDb::~FfiDb()
    {
        if (db_ptr != nullptr)
        {
            unwrap_ffi_result<FfiNoop>(ytflow_db_free(db_ptr));
        }
    }

    FfiConn FfiDb::Connect() const &
    {
        return unwrap_ffi_result<FfiConn>(ytflow_db_conn_new(db_ptr));
    }

    FfiConn::FfiConn(FfiConn &&that) noexcept : conn_ptr(std::exchange(that.conn_ptr, nullptr))
    {
    }
    std::vector<FfiProfile> FfiConn::GetProfiles() &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProfile>>(ytflow_profiles_get_all(conn_ptr));
    }
    uint32_t FfiConn::CreateProfile(const char *name, const char *locale) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(ytflow_profile_create(name, locale, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::DeleteProfile(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_profile_delete(id, conn_ptr));
    }
    void FfiConn::UpdateProfile(uint32_t id, const char *name, const char *locale) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_profile_update(id, name, locale, conn_ptr));
    }
    std::vector<FfiPlugin> FfiConn::GetEntryPluginsByProfile(uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiPlugin>>(ytflow_plugins_get_entry(profileId, conn_ptr));
    }
    std::vector<FfiPlugin> FfiConn::GetPluginsByProfile(uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiPlugin>>(ytflow_plugins_get_by_profile(profileId, conn_ptr));
    }
    void FfiConn::SetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_plugin_set_as_entry(pluginId, profileId, conn_ptr));
    }
    void FfiConn::UnsetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_plugin_unset_as_entry(pluginId, profileId, conn_ptr));
    }
    void FfiConn::DeletePlugin(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_plugin_delete(id, conn_ptr));
    }
    uint32_t FfiConn::CreatePlugin(uint32_t profileId, char const *name, char const *desc, char const *plugin,
                                   uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(
            ytflow_plugin_create(profileId, name, desc, plugin, pluginVersion, param, paramLen, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::UpdatePlugin(uint32_t id, uint32_t profileId, char const *name, char const *desc, char const *plugin,
                               uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_plugin_update(id, profileId, name, desc, plugin, pluginVersion, param, paramLen, conn_ptr));
    }
    std::vector<FfiProxyGroup> FfiConn::GetProxyGroups() &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProxyGroup>>(ytflow_proxy_group_get_all(conn_ptr));
    }
    FfiProxyGroup FfiConn::GetProxyGroupById(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<FfiProxyGroup>(ytflow_proxy_group_get_by_id(id, conn_ptr));
    }
    void FfiConn::RenameProxyGroup(uint32_t id, char const *name) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_group_rename(id, name, conn_ptr));
    }
    void FfiConn::DeleteProxyGroup(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_group_delete(id, conn_ptr));
    }
    uint32_t FfiConn::CreateProxyGroup(char const *name, char const *type) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(ytflow_proxy_group_create(name, type, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    uint32_t FfiConn::CreateProxySubscriptionGroup(char const *name, char const *format, char const *url) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] =
            unwrap_ffi_result<FfiNoop>(ytflow_proxy_group_create_subscription(name, format, url, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    std::vector<FfiProxy> FfiConn::GetProxiesByProxyGroup(uint32_t proxyGroupId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProxy>>(ytflow_proxy_get_by_proxy_group(proxyGroupId, conn_ptr));
    }
    FfiProxyGroupSubscription FfiConn::GetProxySubscriptionByProxyGroup(uint32_t proxyGroupId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<FfiProxyGroupSubscription>(
            ytflow_proxy_subscription_query_by_proxy_group_id(proxyGroupId, conn_ptr));
    }
    void FfiConn::UpdateProxySubscriptionRetrievedByProxyGroup(uint32_t proxyGroupId,
                                                               std::optional<uint64_t> uploadBytes,
                                                               std::optional<uint64_t> downloadBytes,
                                                               std::optional<uint64_t> totalBytes,
                                                               char const *expiresAt) &
    {
        uint64_t const *uploadBytesPtr{nullptr}, *downloadBytesPtr{nullptr}, *totalBytesPtr{nullptr};
        if (uploadBytes.has_value())
        {
            uploadBytesPtr = &*uploadBytes;
        }
        if (downloadBytes.has_value())
        {
            downloadBytesPtr = &*downloadBytes;
        }
        if (totalBytes.has_value())
        {
            totalBytesPtr = &*totalBytes;
        }
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_subscription_update_retrieved_by_proxy_group_id(
            proxyGroupId, uploadBytesPtr, downloadBytesPtr, totalBytesPtr, expiresAt, conn_ptr));
    }
    uint32_t FfiConn::CreateProxy(uint32_t proxyGroupId, char const *name, uint8_t const *proxy, size_t proxyLen,
                                  uint16_t proxyVersion) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(
            ytflow_proxy_create(proxyGroupId, name, proxy, proxyLen, proxyVersion, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::UpdateProxy(uint32_t proxyId, char const *name, uint8_t const *proxy, size_t proxyLen,
                              uint16_t proxyVersion) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_update(proxyId, name, proxy, proxyLen, proxyVersion, conn_ptr));
    }
    void FfiConn::DeleteProxy(uint32_t proxyId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_delete(proxyId, conn_ptr));
    }
    void FfiConn::ReorderProxy(uint32_t proxyGroupId, int32_t rangeStartOrder, int32_t rangeEndOrder, int32_t moves) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_proxy_reorder(proxyGroupId, rangeStartOrder, rangeEndOrder, moves, conn_ptr));
    }
    void FfiConn::BatchUpdateProxyInGroup(uint32_t proxyGroupId, uint8_t const *newProxyBuf, size_t newProxyBufLen) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_proxy_batch_update_by_group(proxyGroupId, newProxyBuf, newProxyBufLen, conn_ptr));
    }
    std::vector<FfiResource> FfiConn::GetResources() &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiResource>>(ytflow_resource_get_all(conn_ptr));
    }
    void FfiConn::DeleteResource(uint32_t resourceId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_resource_delete(resourceId, conn_ptr));
    }
    uint32_t FfiConn::CreateResourceWithUrl(char const *key, char const *type, char const *local_file,
                                            char const *url) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] =
            unwrap_ffi_result<FfiNoop>(ytflow_resource_create_with_url(key, type, local_file, url, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    uint32_t FfiConn::CreateResourceWithGitHubRelease(char const *key, char const *type, char const *local_file,
                                                      char const *github_username, char const *github_repo,
                                                      char const *asset_name) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(ytflow_resource_create_with_github_release(
            key, type, local_file, github_username, github_repo, asset_name, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    FfiResourceUrl FfiConn::GetResourceUrlByResourceId(uint32_t resourceId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<FfiResourceUrl>(ytflow_resource_url_query_by_resource_id(resourceId, conn_ptr));
    }
    void FfiConn::UpdateResourceUrlRetrievedByResourceId(uint32_t resourceId, char const *etag,
                                                         char const *lastModified) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_resource_url_update_retrieved_by_resource_id(resourceId, etag, lastModified, conn_ptr));
    }
    FfiResourceGitHubRelease FfiConn::GetResourceGitHubReleaseByResourceId(uint32_t resourceId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<FfiResourceGitHubRelease>(
            ytflow_resource_github_release_query_by_resource_id(resourceId, conn_ptr));
    }
    void FfiConn::UpdateResourceGitHubReleaseRetrievedByResourceId(uint32_t resourceId, char const *gitTag,
                                                                   char const *releaseTitle) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_resource_github_release_update_retrieved_by_resource_id(resourceId, gitTag, releaseTitle, conn_ptr));
    }

    FfiConn FfiConn::from_ffi(void *ptr1, uintptr_t)
    {
        return FfiConn(static_cast<ytflow_core::ytflow_connection *>(ptr1));
    }
    FfiConn::~FfiConn()
    {
        if (conn_ptr != nullptr)
        {
            unwrap_ffi_result<FfiNoop>(ytflow_db_conn_free(conn_ptr));
        }
    }
    std::string GetYtFlowCoreVersion()
    {
        return std::string(ytflow_core::ytflow_get_version());
    }
}
