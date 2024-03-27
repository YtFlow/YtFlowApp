#pragma once
#include "IdentityConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct IdentityConverter : IdentityConverterT<IdentityConverter>
    {
        IdentityConverter() = default;

        winrt::Windows::Foundation::IInspectable Convert(winrt::Windows::Foundation::IInspectable const &value,
                                                         winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                         winrt::Windows::Foundation::IInspectable const &parameter,
                                                         hstring const &language);
        winrt::Windows::Foundation::IInspectable ConvertBack(
            winrt::Windows::Foundation::IInspectable const &value,
            winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
            winrt::Windows::Foundation::IInspectable const &parameter, hstring const &language);
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct IdentityConverter : IdentityConverterT<IdentityConverter, implementation::IdentityConverter>
    {
    };
}
