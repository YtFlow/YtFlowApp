#include "pch.h"
#include "PluginTypeToDescConverter.h"
#include "PluginTypeToDescConverter.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::YtFlowApp::implementation
{
    IInspectable PluginTypeToDescConverter::Convert(IInspectable const &value, TypeName const & /* targetType */,
                                                    IInspectable const & /* parameter */,
                                                    hstring const & /* language */)
    {
        auto const pluginType{unbox_value<hstring>(value)};
        auto const it{DescMap.find(pluginType)};
        if (it == DescMap.end())
        {
            return value;
        }
        return box_value(it->second);
    }
    IInspectable PluginTypeToDescConverter::ConvertBack(IInspectable const & /* value */,
                                                        TypeName const & /* targetType */,
                                                        IInspectable const & /* parameter */,
                                                        hstring const & /* language */)
    {
        throw hresult_not_implemented();
    }
}
