#pragma once
#include "PluginTypeToDescConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct PluginTypeToDescConverter : PluginTypeToDescConverterT<PluginTypeToDescConverter>
    {
        PluginTypeToDescConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const &value,
                                                  Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                  Windows::Foundation::IInspectable const &parameter,
                                                  hstring const &language);
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const &value,
                                                      Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                      Windows::Foundation::IInspectable const &parameter,
                                                      hstring const &language);
        static std::map<hstring, Windows::Foundation::IInspectable> const DescMap;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct PluginTypeToDescConverter
        : PluginTypeToDescConverterT<PluginTypeToDescConverter, implementation::PluginTypeToDescConverter>
    {
    };
}
