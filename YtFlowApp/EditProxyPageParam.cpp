#include "pch.h"
#include "EditProxyPageParam.h"
#include "EditProxyPageParam.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    winrt::event_token EditProxyPageParam::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &)
    {
        throw hresult_not_implemented();
    }
    void EditProxyPageParam::PropertyChanged(winrt::event_token const &)
    {
        throw hresult_not_implemented();
    }
}
