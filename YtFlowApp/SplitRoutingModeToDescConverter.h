#pragma once
#include "SplitRoutingModeToDescConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct SplitRoutingModeToDescConverter : SplitRoutingModeToDescConverterT<SplitRoutingModeToDescConverter>
    {
        SplitRoutingModeToDescConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const &value,
                                                  Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                  Windows::Foundation::IInspectable const &parameter,
                                                  hstring const &language) const;
        Windows::Foundation::IInspectable ConvertBack(
            Windows::Foundation::IInspectable const &value,
            Windows::UI::Xaml::Interop::TypeName const &targetType,
            Windows::Foundation::IInspectable const &parameter, hstring const &language) const;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct SplitRoutingModeToDescConverter
        : SplitRoutingModeToDescConverterT<SplitRoutingModeToDescConverter,
                                           implementation::SplitRoutingModeToDescConverter>
    {
    };
}
