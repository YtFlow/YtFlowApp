#include "pch.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    std::map<uint32_t, std::string_view> errorFmtStrings = {{1, ""}};
    const char *FfiException::what() const throw()
    {
        return msg.c_str();
    }
    FfiException::FfiException(ytflow_core::FfiResult const &r)
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
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_db_free(db_ptr));
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
        return FfiDb(static_cast<ytflow_core::Database *>(ptr1));
    }
    FfiDb::~FfiDb()
    {
        if (db_ptr != nullptr)
        {
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_db_free(db_ptr));
        }
    }

    FfiConn FfiDb::Connect() const &
    {
        return unwrap_ffi_result<FfiConn>(ytflow_core::ytflow_db_conn_new(db_ptr));
    }

    FfiConn::FfiConn(FfiConn &&that) noexcept : conn_ptr(std::exchange(that.conn_ptr, nullptr))
    {
    }
    std::vector<FfiProfile> FfiConn::GetProfiles() &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProfile>>(ytflow_core::ytflow_profiles_get_all(conn_ptr));
    }
    uint32_t FfiConn::CreateProfile(const char *name, const char *locale) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] =
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profile_create(name, locale, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::DeleteProfile(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profile_delete(id, conn_ptr));
    }
    void FfiConn::UpdateProfile(uint32_t id, const char *name, const char *locale) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profile_update(id, name, locale, conn_ptr));
    }
    std::vector<FfiPlugin> FfiConn::GetEntryPluginsByProfile(uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiPlugin>>(ytflow_core::ytflow_plugins_get_entry(profileId, conn_ptr));
    }
    std::vector<FfiPlugin> FfiConn::GetPluginsByProfile(uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiPlugin>>(
            ytflow_core::ytflow_plugins_get_by_profile(profileId, conn_ptr));
    }
    void FfiConn::SetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_plugin_set_as_entry(pluginId, profileId, conn_ptr));
    }
    void FfiConn::UnsetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_plugin_unset_as_entry(pluginId, profileId, conn_ptr));
    }
    void FfiConn::DeletePlugin(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_plugin_delete(id, conn_ptr));
    }
    uint32_t FfiConn::CreatePlugin(uint32_t profileId, char const *name, char const *desc, char const *plugin,
                                   uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(
            ytflow_core::ytflow_plugin_create(profileId, name, desc, plugin, pluginVersion, param, paramLen, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::UpdatePlugin(uint32_t id, uint32_t profileId, char const *name, char const *desc, char const *plugin,
                               uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_plugin_update(id, profileId, name, desc, plugin, pluginVersion,
                                                                     param, paramLen, conn_ptr));
    }
    std::vector<FfiProxyGroup> FfiConn::GetProxyGroups() &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProxyGroup>>(ytflow_core::ytflow_proxy_group_get_all(conn_ptr));
    }
    FfiProxyGroup FfiConn::GetProxyGroupById(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<FfiProxyGroup>(ytflow_core::ytflow_proxy_group_get_by_id(id, conn_ptr));
    }
    void FfiConn::RenameProxyGroup(uint32_t id, char const *name) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_proxy_group_rename(id, name, conn_ptr));
    }
    void FfiConn::DeleteProxyGroup(uint32_t id) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_proxy_group_delete(id, conn_ptr));
    }
    uint32_t FfiConn::CreateProxyGroup(char const *name, char const *type) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] =
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_proxy_group_create(name, type, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    std::vector<FfiProxy> FfiConn::GetProxiesByProxyGroup(uint32_t proxyGroupId) &
    {
        std::lock_guard _guard(conn_mu);
        return unwrap_ffi_buffer<std::vector<FfiProxy>>(
            ytflow_core::ytflow_proxy_get_by_proxy_group(proxyGroupId, conn_ptr));
    }
    uint32_t FfiConn::CreateProxy(uint32_t proxyGroupId, char const *name, uint8_t const *proxy, size_t proxyLen,
                                  uint16_t proxyVersion) &
    {
        std::lock_guard _guard(conn_mu);
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(
            ytflow_core::ytflow_proxy_create(proxyGroupId, name, proxy, proxyLen, proxyVersion, conn_ptr));
        return (uint32_t)((uintptr_t)ptrRaw & 0xFFFFFFFF);
    }
    void FfiConn::UpdateProxy(uint32_t proxyId, char const *name, uint8_t const *proxy, size_t proxyLen,
                              uint16_t proxyVersion) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_core::ytflow_proxy_update(proxyId, name, proxy, proxyLen, proxyVersion, conn_ptr));
    }
    void FfiConn::DeleteProxy(uint32_t proxyId) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_proxy_delete(proxyId, conn_ptr));
    }
    void FfiConn::ReorderProxy(uint32_t proxyGroupId, int32_t rangeStartOrder, int32_t rangeEndOrder, int32_t moves) &
    {
        std::lock_guard _guard(conn_mu);
        unwrap_ffi_result<FfiNoop>(
            ytflow_core::ytflow_proxy_reorder(proxyGroupId, rangeStartOrder, rangeEndOrder, moves, conn_ptr));
    }

    FfiConn FfiConn::from_ffi(void *ptr1, uintptr_t)
    {
        return FfiConn(static_cast<ytflow_core::Connection *>(ptr1));
    }
    FfiConn::~FfiConn()
    {
        if (conn_ptr != nullptr)
        {
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_db_conn_free(conn_ptr));
        }
    }
    std::string GetYtFlowCoreVersion()
    {
        return std::string(ytflow_core::ytflow_get_version());
    }
}
