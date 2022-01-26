#include "pch.h"

#include "CoreFfi.h"
#include <format>
#include <nlohmann/json.hpp>

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

    FfiConn::FfiConn(FfiConn &&that) noexcept : conn_ptr(that.conn_ptr)
    {
        that.conn_ptr = nullptr;
    }
    std::vector<FfiProfile> FfiConn::GetProfiles() const &
    {
        const auto [ptrRaw, metaRaw] = unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profiles_get_all(conn_ptr));
        auto ptr = (const uint8_t *)ptrRaw;
        auto meta = (size_t)metaRaw;
        std::vector<FfiProfile> json = nlohmann::json::from_cbor(ptr, ptr + meta);
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_buffer_free(ptrRaw, metaRaw));
        return json;
    }
    uint32_t FfiConn::CreateProfile(const char *name, const char *locale) const &
    {
        const auto [ptrRaw, metaRaw] =
            unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profile_create(name, locale, conn_ptr));
        return (uint32_t)ptrRaw;
    }
    void FfiConn::DeleteProfile(uint32_t id) const &
    {
        unwrap_ffi_result<FfiNoop>(ytflow_core::ytflow_profile_delete(id, conn_ptr));
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
}
