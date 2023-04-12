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

        FfiException(ytflow_core::FfiResult const &r);
        const char *what() const throw() override;
    };
    template <typename R> auto unwrap_ffi_result(ytflow_core::FfiResult r) -> decltype(R::from_ffi(nullptr, NULL))
    {
        if (r.code == 0)
        {
            return R::from_ffi(r.data.res._0, r.data.res._1);
        }
        else
        {
            FfiException ex(r);
            ytflow_core::ytflow_result_free(&r);
            throw ex;
        }
    }
    template <typename R> R unwrap_ffi_buffer(ytflow_core::FfiResult r)
    {
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(r);
        auto ptr = (const uint8_t *)ptrRaw;
        auto meta = (size_t)metaRaw;
        R json = nlohmann::json::from_cbor(ptr, ptr + meta);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_buffer_free(ptrRaw, metaRaw));
        return json;
    }
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
    struct FfiConn final
    {
        FfiConn(ytflow_core::Connection *conn) noexcept : conn_ptr(conn)
        {
        }
        FfiConn(FfiConn &&) noexcept;
        FfiConn(const FfiConn &) = delete;
        FfiConn &operator=(const FfiConn &) = delete;

        std::vector<FfiProfile> GetProfiles() const &;
        void DeleteProfile(uint32_t id) const &;
        uint32_t CreateProfile(const char *name, const char *locale) const &;
        void UpdateProfile(uint32_t id, const char *name, const char *locale) const &;
        std::vector<FfiPlugin> GetPluginsByProfile(uint32_t profileId) const &;
        std::vector<FfiPlugin> GetEntryPluginsByProfile(uint32_t profileId) const &;
        void SetPluginAsEntry(uint32_t pluginId, uint32_t profileId) const &;
        void UnsetPluginAsEntry(uint32_t pluginId, uint32_t profileId) const &;
        void DeletePlugin(uint32_t id) const &;
        uint32_t CreatePlugin(uint32_t profileId, char const *name, char const *desc, char const *plugin,
                              uint16_t pluginVersion, uint8_t const *param, size_t paramLen) const &;
        void UpdatePlugin(uint32_t id, uint32_t profileId, char const *name, char const *desc, char const *plugin,
                          uint16_t pluginVersion, uint8_t const *param, size_t paramLen) const &;

        static FfiConn from_ffi(void *ptr1, uintptr_t);
        ~FfiConn();

      private:
        ytflow_core::Connection *conn_ptr{nullptr};
    };
    struct FfiDb final
    {
        FfiDb() = default;
        FfiDb(ytflow_core::Database *db) noexcept : db_ptr(db)
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
        ytflow_core::Database *db_ptr{nullptr};
    };
    inline FfiDb FfiDbInstance;
}
