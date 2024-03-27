#include "pch.h"
#include "IdentityConverter.h"
#include "IdentityConverter.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    using namespace Windows::Foundation;
    using Windows::UI::Xaml::Interop::TypeName;

    IInspectable IdentityConverter::Convert(IInspectable const &value, TypeName const &, IInspectable const &,
                                            hstring const &)
    {
        return value;
    }
    IInspectable IdentityConverter::ConvertBack(IInspectable const &value, TypeName const &, IInspectable const &,
                                                hstring const &)
    {
        return value;
    }
}
