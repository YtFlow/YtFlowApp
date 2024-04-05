#include "pch.h"
#include "StringUtil.h"

namespace winrt::YtFlowApp::implementation
{
    std::string_view TrimSpaces(std::string_view str)
    {
        constexpr char const *SPACES = " \r\n\t\v\f";
        size_t start = str.find_first_not_of(SPACES);
        if (start == std::string_view::npos)
        {
            return {};
        }
        size_t end = str.find_last_not_of(SPACES);
        return str.substr(start, end - start + 1);
    }
}
