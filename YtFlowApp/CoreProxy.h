#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace winrt::YtFlowApp::implementation
{

    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertShareLinkToProxy(std::string const &link);
    std::optional<std::string> ConvertProxyToShareLink(std::string_view name, std::span<uint8_t const> proxy);
}
