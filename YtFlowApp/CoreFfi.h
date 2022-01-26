#pragma once
#include <nlohmann/json.hpp>
#include <ytflow_core.h>

namespace winrt::YtFlowApp::implementation
{
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
    struct FfiNoop
    {
        inline static std::pair<void *, uintptr_t> from_ffi(void *ptr, uintptr_t meta)
        {
            return std::make_pair(ptr, meta);
        }
    };
    struct FfiProfile
    {
        uint32_t id;
        std::string name;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProfile, id, name)
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
