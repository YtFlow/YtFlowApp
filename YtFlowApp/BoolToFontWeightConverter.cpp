#include "pch.h"
#include "BoolToFontWeightConverter.h"
#include "BoolToFontWeightConverter.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Text;

namespace winrt::YtFlowApp::implementation
{
    IInspectable BoolToFontWeightConverter::Convert(IInspectable const &value,
                                                    TypeName const &,
                                                    IInspectable const &,
                                                    hstring const &)
    {
        if (value == nullptr)
        {
            return m_defaultFontWeight;
        }

        if (value.try_as<bool>().value_or(false))
        {
            return m_boldFontWeight;
        }
        return m_defaultFontWeight;
    }

    IInspectable BoolToFontWeightConverter::ConvertBack(IInspectable const &,
                                                        TypeName const &,
                                                        IInspectable const &,
                                                        hstring const &)
    {
        throw hresult_not_implemented();
    }
}
