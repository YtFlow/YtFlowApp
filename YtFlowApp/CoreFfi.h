#pragma once
#include <ytflow_core.h>

namespace winrt::YtFlowApp::implementation
{
    constexpr const uint32_t INVALID_DB_ID = 0xFFFFFFFF;
    struct FfiNoop
    {
        inline static std::pair<void *, uintptr_t> from_ffi(void *ptr, uintptr_t meta)
        {
            return std::make_pair(ptr, meta);
        }
    };
    struct FfiException : public std::exception
    {
        std::string msg;

        FfiException(ytflow_core::ytflow_result const &r);
        const char *what() const throw() override;
    };
    template <typename R> auto unwrap_ffi_result(ytflow_core::ytflow_result r) -> decltype(R::from_ffi(nullptr, NULL))
    {
        if (r.code == 0)
        {
            return R::from_ffi(r.data.res._0, r.data.res._1);
        }
        else
        {
            FfiException ex(r);
            ytflow_result_free(&r);
            throw ex;
        }
    }
    template <typename R> R unwrap_ffi_buffer(ytflow_core::ytflow_result r)
    {
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(r);
        auto ptr = (const uint8_t *)ptrRaw;
        auto meta = (size_t)metaRaw;
        R json = nlohmann::json::from_cbor(ptr, ptr + meta);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_buffer_free(ptrRaw, metaRaw));
        return json;
    }
    std::vector<uint8_t> unwrap_ffi_byte_buffer(ytflow_core::ytflow_result r);
    std::string unwrap_ffi_string(ytflow_core::ytflow_result r);

    struct FfiProfile
    {
        uint32_t id{};
        std::string name;
        std::string locale;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProfile, id, name, locale)
    struct FfiPluginDescriptor
    {
        std::string descriptor;
        // TODO: bitflags Serializing
        // std::string type;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiPluginDescriptor, descriptor)
    struct FfiPluginVerifyResult
    {
        std::vector<FfiPluginDescriptor> required;
        std::vector<FfiPluginDescriptor> provides;
    };
    inline void from_json(nlohmann::json const &json, FfiPluginVerifyResult &r)
    {
        json.at("requires").get_to(r.required);
        json.at("provides").get_to(r.provides);
    }
    struct FfiPlugin
    {
        uint32_t id{INVALID_DB_ID};
        std::string name;
        std::string desc;
        std::string plugin;
        uint16_t plugin_version{0};
        std::vector<uint8_t> param;

        static FfiPluginVerifyResult verify(char const *plugin, uint16_t plugin_version, uint8_t const *param,
                                            size_t param_len);
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiPlugin, id, name, desc, plugin, plugin_version, param)
    struct FfiProxyGroup
    {
        uint32_t id{};
        std::string name;
        std::string type;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyGroup, id, name, type)
    struct FfiProxyGroupSubscription
    {
        std::string format;
        std::string url;
        std::optional<uint64_t> upload_bytes_used;
        std::optional<uint64_t> download_bytes_used;
        std::optional<uint64_t> bytes_total;
        std::optional<std::string> expires_at;
        std::optional<std::string> retrieved_at;
    };
    inline void from_json(nlohmann::json const &json, FfiProxyGroupSubscription &r)
    {
        json.at("format").get_to(r.format);
        json.at("url").get_to(r.url);
        if (nlohmann::json const uploadBytesUsedDoc = json.value("upload_bytes_used", nlohmann::json());
            uploadBytesUsedDoc != nullptr)
        {
            r.upload_bytes_used = {uploadBytesUsedDoc.get<uint64_t>()};
        }
        if (nlohmann::json const downloadBytesUsedDoc = json.value("download_bytes_used", nlohmann::json());
            downloadBytesUsedDoc != nullptr)
        {
            r.download_bytes_used = {downloadBytesUsedDoc.get<uint64_t>()};
        }
        if (nlohmann::json const bytesTotalDoc = json.value("bytes_total", nlohmann::json()); bytesTotalDoc != nullptr)
        {
            r.bytes_total = {bytesTotalDoc.get<uint64_t>()};
        }
        if (nlohmann::json const expiresAtDoc = json.value("expires_at", nlohmann::json()); expiresAtDoc != nullptr)
        {
            r.expires_at = {expiresAtDoc.get<std::string>()};
        }
        if (nlohmann::json const retrievedAtDoc = json.value("retrieved_at", nlohmann::json());
            retrievedAtDoc != nullptr)
        {
            r.retrieved_at = {retrievedAtDoc.get<std::string>()};
        }
    }
    struct FfiDataProxy
    {
        uint32_t id{};
        std::string name;
        int32_t order_num{};
        std::vector<uint8_t> proxy;
        uint16_t proxy_version{0};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiDataProxy, id, name, order_num, proxy, proxy_version)
    struct FfiResource
    {
        uint32_t id{};
        std::string key;
        std::string type;
        std::string local_file;
        std::string remote_type;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiResource, id, key, type, local_file, remote_type)
    struct FfiResourceUrl
    {
        uint32_t id{};
        std::string url;
        std::optional<std::string> etag;
        std::optional<std::string> last_modified;
        std::optional<std::string> retrieved_at;
    };
    inline void from_json(nlohmann::json const &json, FfiResourceUrl &r)
    {
        json.at("id").get_to(r.id);
        json.at("url").get_to(r.url);
        if (!json.contains("retrieved_at") || json.at("retrieved_at") == nullptr)
        {
            return;
        }
        if (nlohmann::json const etagDoc = json.value("etag", nlohmann::json()); etagDoc != nullptr)
        {
            r.etag = {etagDoc.get<std::string>()};
        }
        if (nlohmann::json const lastModifiedDoc = json.value("last_modified", nlohmann::json());
            lastModifiedDoc != nullptr)
        {
            r.last_modified = {lastModifiedDoc.get<std::string>()};
        }
        if (nlohmann::json const retrievedAtDoc = json.value("retrieved_at", nlohmann::json());
            retrievedAtDoc != nullptr)
        {
            r.retrieved_at = {retrievedAtDoc.get<std::string>()};
        }
    }
    struct FfiResourceGitHubRelease
    {
        uint32_t id{};
        std::string github_username;
        std::string github_repo;
        std::string asset_name;
        std::optional<std::string> git_tag;
        std::optional<std::string> release_title;
        std::optional<std::string> retrieved_at;
    };
    inline void from_json(nlohmann::json const &json, FfiResourceGitHubRelease &r)
    {
        json.at("id").get_to(r.id);
        json.at("github_username").get_to(r.github_username);
        json.at("github_repo").get_to(r.github_repo);
        json.at("asset_name").get_to(r.asset_name);
        if (!json.contains("retrieved_at") || json.at("retrieved_at") == nullptr)
        {
            return;
        }
        if (nlohmann::json const gitTagDoc = json.value("git_tag", nlohmann::json()); gitTagDoc != nullptr)
        {
            r.git_tag = {gitTagDoc.get<std::string>()};
        }
        if (nlohmann::json const releaseTitleDoc = json.value("release_title", nlohmann::json());
            releaseTitleDoc != nullptr)
        {
            r.release_title = {releaseTitleDoc.get<std::string>()};
        }
        if (nlohmann::json const retrievedAtDoc = json.value("retrieved_at", nlohmann::json());
            retrievedAtDoc != nullptr)
        {
            r.retrieved_at = {retrievedAtDoc.get<std::string>()};
        }
    }

    struct FfiConn final
    {
        FfiConn(ytflow_core::ytflow_connection *conn) noexcept : conn_ptr(conn)
        {
        }
        FfiConn(FfiConn &&) noexcept;
        FfiConn(const FfiConn &) = delete;
        FfiConn &operator=(const FfiConn &) = delete;

        std::vector<FfiProfile> GetProfiles() &;
        void DeleteProfile(uint32_t id) &;
        uint32_t CreateProfile(const char *name, const char *locale) &;
        void UpdateProfile(uint32_t id, const char *name, const char *locale) &;
        std::vector<FfiPlugin> GetPluginsByProfile(uint32_t profileId) &;
        std::vector<FfiPlugin> GetEntryPluginsByProfile(uint32_t profileId) &;
        void SetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &;
        void UnsetPluginAsEntry(uint32_t pluginId, uint32_t profileId) &;
        void DeletePlugin(uint32_t id) &;
        uint32_t CreatePlugin(uint32_t profileId, char const *name, char const *desc, char const *plugin,
                              uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &;
        void UpdatePlugin(uint32_t id, uint32_t profileId, char const *name, char const *desc, char const *plugin,
                          uint16_t pluginVersion, uint8_t const *param, size_t paramLen) &;
        std::vector<FfiProxyGroup> GetProxyGroups() &;
        FfiProxyGroup GetProxyGroupById(uint32_t id) &;
        void RenameProxyGroup(uint32_t id, char const *name) &;
        void DeleteProxyGroup(uint32_t id) &;
        uint32_t CreateProxyGroup(char const *name, char const *type) &;
        uint32_t CreateProxySubscriptionGroup(char const *name, char const *format, char const *url) &;
        std::vector<FfiDataProxy> GetProxiesByProxyGroup(uint32_t proxyGroupId) &;
        FfiProxyGroupSubscription GetProxySubscriptionByProxyGroup(uint32_t proxyGroupId) &;
        void UpdateProxySubscriptionRetrievedByProxyGroup(uint32_t proxyGroupId, std::optional<uint64_t> uploadBytes,
                                                          std::optional<uint64_t> downloadBytes,
                                                          std::optional<uint64_t> totalBytes, char const *expiresAt) &;
        uint32_t CreateProxy(uint32_t proxyGroupId, char const *name, uint8_t const *proxy, size_t proxyLen,
                             uint16_t proxyVersion) &;
        void UpdateProxy(uint32_t proxyId, char const *name, uint8_t const *proxy, size_t proxyLen,
                         uint16_t proxyVersion) &;
        void DeleteProxy(uint32_t proxyId) &;
        void ReorderProxy(uint32_t proxyGroupId, int32_t rangeStartOrder, int32_t rangeEndOrder, int32_t moves) &;
        void BatchUpdateProxyInGroup(uint32_t proxyGroupId, uint8_t const *newProxyBuf, size_t newProxyBufLen) &;
        std::vector<FfiResource> GetResources() &;
        void DeleteResource(uint32_t resourceId) &;
        uint32_t CreateResourceWithUrl(char const *key, char const *type, char const *local_file, char const *url) &;
        uint32_t CreateResourceWithGitHubRelease(char const *key, char const *type, char const *local_file,
                                                 char const *github_username, char const *github_repo,
                                                 char const *asset_name) &;
        FfiResourceUrl GetResourceUrlByResourceId(uint32_t resourceId) &;
        void UpdateResourceUrlRetrievedByResourceId(uint32_t resourceId, char const *etag, char const *lastModified) &;
        FfiResourceGitHubRelease GetResourceGitHubReleaseByResourceId(uint32_t resourceId) &;
        void UpdateResourceGitHubReleaseRetrievedByResourceId(uint32_t resourceId, char const *gitTag,
                                                              char const *releaseTitle) &;

        static FfiConn from_ffi(void *ptr1, uintptr_t);
        ~FfiConn();

      private:
        std::mutex conn_mu{};
        ytflow_core::ytflow_connection *conn_ptr{nullptr};
    };
    struct FfiDb final
    {
        FfiDb() = default;
        FfiDb(ytflow_core::ytflow_database *db) noexcept : db_ptr(db)
        {
        }
        FfiDb(FfiDb &&) noexcept;
        FfiDb(const FfiDb &) = delete;
        FfiDb &operator=(const FfiDb &) = delete;
        FfiDb &operator=(FfiDb &&);
        bool operator==(std::nullptr_t) const noexcept;

        static FfiDb Open(std::wstring_view path);
        FfiConn Connect() const &;

        static FfiDb from_ffi(void *ptr1, uintptr_t);
        ~FfiDb();

      private:
        ytflow_core::ytflow_database *db_ptr{nullptr};
    };
    inline FfiDb FfiDbInstance;
    std::string GetYtFlowCoreVersion();
}
