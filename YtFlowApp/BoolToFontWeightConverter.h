#pragma once
#include "BoolToFontWeightConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct BoolToFontWeightConverter : BoolToFontWeightConverterT<BoolToFontWeightConverter>
    {
        BoolToFontWeightConverter() = default;

        winrt::Windows::Foundation::IInspectable Convert(winrt::Windows::Foundation::IInspectable const &value,
                                                         winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                         winrt::Windows::Foundation::IInspectable const &parameter,
                                                         hstring const &language);
        winrt::Windows::Foundation::IInspectable ConvertBack(
            winrt::Windows::Foundation::IInspectable const &value,
            winrt::Windows::UI::Xaml::Interop::TypeName const &targetType,
            winrt::Windows::Foundation::IInspectable const &parameter, hstring const &language);

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
