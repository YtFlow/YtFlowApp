#include "pch.h"
#include "BoolToObjectConverter.h"
#include "BoolToObjectConverter.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Text;

namespace winrt::YtFlowApp::implementation
{
    Windows::UI::Xaml::Visibility BoolToObjectConverter::ToVisibility(bool input)
    {
        return input ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
    }
    Windows::UI::Xaml::Visibility BoolToObjectConverter::NullabilityToVisibility(IInspectable const &input)
    {
        if (auto const val = input.try_as<bool>(); val.has_value())
        {
            return ToVisibility(val.value());
        }
        return ToVisibility(input != nullptr);
    }
    IInspectable BoolToObjectConverter::Convert(IInspectable const &value, TypeName const &, IInspectable const &,
                                                hstring const &) const
    {
        auto const val = value.try_as<bool>();
        if (!val.has_value())
        {
            return DefaultObject();
        }
        if (val.value())
        {
            return TrueObject();
        }
        return FalseObject();
    }

    IInspectable BoolToObjectConverter::ConvertBack(IInspectable const &, TypeName const &, IInspectable const &,
                                                    hstring const &)
    {
        throw hresult_not_implemented();
    }
    IInspectable BoolToObjectConverter::TrueObject() const
    {
        return m_trueObject;
    }
    void BoolToObjectConverter::TrueObject(IInspectable const &value)
    {
        m_trueObject = value;
    }
    IInspectable BoolToObjectConverter::FalseObject() const
    {
        return m_falseObject;
    }
    void BoolToObjectConverter::FalseObject(IInspectable const &value)
    {
        m_falseObject = value;
    }
    IInspectable BoolToObjectConverter::DefaultObject() const
    {
        return m_defaultObject ? m_defaultObject : m_falseObject;
    }
    void BoolToObjectConverter::DefaultObject(IInspectable const &value)
    {
        m_defaultObject = value;
    }
}
