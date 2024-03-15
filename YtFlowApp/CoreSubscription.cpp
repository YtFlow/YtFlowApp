#include "pch.h"

#include "CoreSubscription.h"

#include "CoreFfi.h"
#include "CoreProxy.h"

namespace winrt::YtFlowApp::implementation
{
    using ytflow_core::ytflow_app_proxy_data_proxy_compose_v1;
    using ytflow_core::ytflow_app_subscription_decode;
    using ytflow_core::ytflow_app_subscription_decode_with_format;
    using ytflow_core::ytflow_app_subscription_userinfo_header_decode;

    struct FfiSubscription
    {
        std::vector<nlohmann::json> proxies;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiSubscription, proxies)
    struct ProxyInput
    {
        std::string name;
        nlohmann::json::binary_t proxy;
        uint16_t proxy_version = 0;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProxyInput, name, proxy, proxy_version)

    void DecodeSubscriptionUserInfoFromCbor(nlohmann::json const &json, DecodedSubscriptionUserInfo &p)
    {
        if (nlohmann::json const uploadBytesUsedDoc = json.value("upload_bytes_used", nlohmann::json());

            uploadBytesUsedDoc != nullptr)
        {
            p.upload_bytes_used = {uploadBytesUsedDoc.get<uint64_t>()};
        }
        if (nlohmann::json const downloadBytesUsedDoc = json.value("download_bytes_used", nlohmann::json());

            downloadBytesUsedDoc != nullptr)
        {
            p.download_bytes_used = {downloadBytesUsedDoc.get<uint64_t>()};
        }
        if (nlohmann::json const bytesTotalDoc = json.value("bytes_total", nlohmann::json());

            bytesTotalDoc != nullptr)
        {
            p.bytes_total = {bytesTotalDoc.get<uint64_t>()};
        }
        if (nlohmann::json const expiresAtDoc = json.value("expires_at", nlohmann::json());

            expiresAtDoc != nullptr)
        {
            p.expires_at = {expiresAtDoc.get<std::string>()};
        }
    }

    DecodedSubscriptionUserInfo DecodeSubscriptionUserInfoFromResponseHeaderValue(std::string const &resValue)
    {
        return unwrap_ffi_buffer<DecodedSubscriptionUserInfo>(
            ytflow_app_subscription_userinfo_header_decode(resValue.c_str()));
    }
    std::optional<std::vector<uint8_t>> DecodeSubscriptionProxies(std::string_view data, char const *&decodedFormat)
    {
        try
        {
            FfiSubscription decoded;
            auto const data_ptr = reinterpret_cast<uint8_t const *>(data.data());
            if (decodedFormat == nullptr)
            {
                decoded = unwrap_ffi_buffer<FfiSubscription>(
                    ytflow_app_subscription_decode(data_ptr, data.size(), &decodedFormat));
            }
            else
            {
                decoded = unwrap_ffi_buffer<FfiSubscription>(ytflow_app_subscription_decode_with_format(
                    reinterpret_cast<uint8_t const *>(data.data()), data.size(), decodedFormat));
            }
            if (decoded.proxies.empty())
            {
                decodedFormat = nullptr;
                return std::nullopt;
            }

            using namespace std::ranges::views;
            using std::ranges::to;
            auto const data_proxies =
                transform(decoded.proxies,
                          [](auto const &proxy) {
                              auto name = proxy.at("name").get<std::string>();
                              auto const proxy_buf = nlohmann::json::to_cbor(proxy);
                              auto data_proxy_buf = unwrap_ffi_byte_buffer(
                                  ytflow_app_proxy_data_proxy_compose_v1(proxy_buf.data(), proxy_buf.size()));
                              return ProxyInput{.name = std::move(name), .proxy = std::move(data_proxy_buf)};
                          }) |
                to<std::vector>();
            return std::make_optional(nlohmann::json::to_cbor(data_proxies));
        }
        catch (FfiException const &)
        {
            decodedFormat = nullptr;
            return std::nullopt;
        }
    }
}
