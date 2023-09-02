#pragma once
#include "SplitRoutingModeToDescConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct SplitRoutingModeToDescConverter : SplitRoutingModeToDescConverterT<SplitRoutingModeToDescConverter>
    {
        SplitRoutingModeToDescConverter() = default;

        winrt::Windows::Foundation::IInspectable Convert(winrt::Windows::Foundation::IInspectable const &value,
                                                         winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                         winrt::Windows::Foundation::IInspectable const &parameter,
                                                         hstring const &language) const;
        winrt::Windows::Foundation::IInspectable ConvertBack(
            winrt::Windows::Foundation::IInspectable const &value,
            winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
            winrt::Windows::Foundation::IInspectable const &parameter, hstring const &language) const;
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
