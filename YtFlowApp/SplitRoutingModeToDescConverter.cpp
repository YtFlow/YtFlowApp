#include "pch.h"
#include "SplitRoutingModeToDescConverter.h"
#include "SplitRoutingModeToDescConverter.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Text;

namespace winrt::YtFlowApp::implementation
{
    winrt::Windows::Foundation::IInspectable SplitRoutingModeToDescConverter::Convert(IInspectable const &value,
                                                                                      TypeName const &,
                                                                                      IInspectable const &,
                                                                                      hstring const &) const
    {
        auto const modeOpt = value.try_as<hstring>();
        if (!modeOpt.has_value())
        {
            return box_value(L"");
        }
        auto const mode = modeOpt.value();

        if (mode == L"all")
        {
            return box_value(L"Connections to all destinations are handled by proxy forwarder.");
        }
        if (mode == L"whitelist")
        {
            return box_value(
                L"Connection requests to Chinese websites, determined by rulesets, are handled by direct forwarder. "
                L"All remaining connections are handled by proxy forwarder. Also called whitelist mode.");
        }
        if (mode == L"blacklist")
        {
            return box_value(
                L"Connection requests to known, overseas websites, determined by rulesets, are handled by proxy "
                L"forwarder. All remaining connections are handled by direct forwarder. Also called blacklist mode.");
        }
        if (mode == L"overseas")
        {
            return box_value(
                L"Connection requests to Chinese websites, determined by rulesets, are handled by proxy forwarder. All "
                L"remaining connections are handled by direct forwarder. Suitable for overseas users.");
        }

        return nullptr;
    }

    winrt::Windows::Foundation::IInspectable SplitRoutingModeToDescConverter::ConvertBack(IInspectable const &,
                                                                                          TypeName const &,
                                                                                          IInspectable const &,
                                                                                          hstring const &) const
    {
        throw hresult_not_implemented();
    }
}
