#pragma once
#include "BoolToObjectConverter.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct BoolToObjectConverter : BoolToObjectConverterT<BoolToObjectConverter>
    {
        BoolToObjectConverter() = default;

        static Windows::UI::Xaml::Visibility ToVisibility(bool input);
        static Windows::UI::Xaml::Visibility NullabilityToVisibility(IInspectable const &input);

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const &value,
                                                  Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                  Windows::Foundation::IInspectable const &parameter,
                                                  hstring const &language) const;
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const &value,
                                                      Windows::UI::Xaml::Interop::TypeName const &targetType,
                                                      Windows::Foundation::IInspectable const &parameter,
                                                      hstring const &language);
        IInspectable TrueObject() const;
        void TrueObject(IInspectable const &value);
        IInspectable FalseObject() const;
        void FalseObject(IInspectable const &value);
        IInspectable DefaultObject() const;
        void DefaultObject(IInspectable const &value);

      private:
        IInspectable m_trueObject = nullptr;
        IInspectable m_falseObject = nullptr;
        IInspectable m_defaultObject = nullptr;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct BoolToObjectConverter : BoolToObjectConverterT<BoolToObjectConverter, implementation::BoolToObjectConverter>
    {
    };
}
