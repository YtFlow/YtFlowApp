#pragma once

#include "CoreProxy.h"

namespace winrt::YtFlowApp::implementation
{
    struct DecodedSubscriptionUserInfo
    {
        std::optional<uint64_t> upload_bytes_used;
        std::optional<uint64_t> download_bytes_used;
        std::optional<uint64_t> bytes_total;
        std::optional<std::string> expires_at;
    };

    void DecodeSubscriptionUserInfoFromCbor(nlohmann::json const &json, DecodedSubscriptionUserInfo &p);
    inline void from_json(nlohmann::json const &json, DecodedSubscriptionUserInfo &p)
    {
        DecodeSubscriptionUserInfoFromCbor(json, p);
    }

    DecodedSubscriptionUserInfo DecodeSubscriptionUserInfoFromResponseHeaderValue(std::string const &resValue);
    std::optional<std::vector<uint8_t>> DecodeSubscriptionProxies(std::string_view data, char const *&decodedFormat);
}
