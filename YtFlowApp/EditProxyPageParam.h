#pragma once
#include "EditProxyPageParam.g.h"

#include "ProxyModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditProxyPageParam : EditProxyPageParamT<EditProxyPageParam>
    {
        EditProxyPageParam() = default;
        EditProxyPageParam(bool isReadonly, com_ptr<ProxyModel> proxy) : IsReadonly(isReadonly), Proxy(proxy)
        {
        }

        bool IsReadonly{};
        com_ptr<ProxyModel> Proxy{nullptr};
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token);
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProxyPageParam : EditProxyPageParamT<EditProxyPageParam, implementation::EditProxyPageParam>
    {
    };
}
