#pragma once

#include "CoreProxy.h"

namespace winrt::YtFlowApp::implementation
{
    constexpr static char SIP008_LITERAL[7] = "sip008";
    struct DecodedSubscriptionUserInfo
    {
        std::optional<uint64_t> upload_bytes_used;
        std::optional<uint64_t> download_bytes_used;
        std::optional<uint64_t> bytes_total;
        std::optional<std::string> expires_at;
    };

    char const *ConvertSubscriptionFormatToStatic(char const *input);
    DecodedSubscriptionUserInfo DecodeSubscriptionUserInfoFromResponseHeaderValue(std::string_view resValue);
    std::optional<std::vector<uint8_t>> DecodeSubscriptionProxies(std::string_view data, char const *&decodedFormat);
}
