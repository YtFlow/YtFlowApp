#pragma once
#include "BoolToFontWeightConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct BoolToFontWeightConverter : BoolToFontWeightConverterT<BoolToFontWeightConverter>
    {
        BoolToFontWeightConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const &value,
                                                  Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                  Windows::Foundation::IInspectable const &parameter,
                                                  hstring const &language);
        Windows::Foundation::IInspectable ConvertBack(
            Windows::Foundation::IInspectable const &value,
            Windows::UI::Xaml::Interop::TypeName const &targetType,
            Windows::Foundation::IInspectable const &parameter, hstring const &language);

      private:
        inline static IInspectable m_defaultFontWeight{box_value(Windows::UI::Text::FontWeights::Normal())};
        inline static IInspectable m_boldFontWeight{box_value(Windows::UI::Text::FontWeights::SemiBold())};
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct BoolToFontWeightConverter
        : BoolToFontWeightConverterT<BoolToFontWeightConverter, implementation::BoolToFontWeightConverter>
    {
    };
}
