#pragma once

#include "EditProxyPage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditProxyPage : EditProxyPageT<EditProxyPage>
    {
        EditProxyPage();

        static Windows::UI::Xaml::DependencyProperty IsDirtyProperty();
        static Windows::UI::Xaml::Visibility IsShadowsocks(hstring const &protocolType) noexcept;
        static Windows::UI::Xaml::Visibility IsTrojan(hstring const &protocolType) noexcept;
        static Windows::UI::Xaml::Visibility IsHttp(hstring const &protocolType) noexcept;
        static Windows::UI::Xaml::Visibility IsSocks5(hstring const &protocolType) noexcept;
        static Windows::UI::Xaml::Visibility IsVMess(hstring const &protocolType) noexcept;
        static Windows::UI::Xaml::Visibility IsHttpObfs(hstring const &obfsType) noexcept;
        static Windows::UI::Xaml::Visibility IsTlsObfs(hstring const &obfsType) noexcept;
        static Windows::UI::Xaml::Visibility IsWebSocket(hstring const &obfsType) noexcept;
        static bool TlsParamsAllowed(bool enableTls, bool isReadonly) noexcept;

        fire_and_forget OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        fire_and_forget OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args);
        void AdaptiveWidth_StateChanged(Windows::Foundation::IInspectable const &sender,
                                        Windows::UI::Xaml::VisualStateChangedEventArgs const &e);
        hstring ProxyName();
        void ProxyName(hstring proxyName);
        Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProxyLegModel> ProxyLegs();
        bool IsUdpSupported() const;
        void IsUdpSupported(bool isUdpSupported);
        bool IsReadonly() const;
        bool IsWritable() const;
        bool IsDirty() const;
        void IsDirty(bool isDirty) const;

      private:
        inline static Windows::UI::Xaml::DependencyProperty m_isDirtyProperty =
            Windows::UI::Xaml::DependencyProperty::Register(L"IsDirty", winrt::xaml_typename<bool>(),
                                                            winrt::xaml_typename<YtFlowApp::EditProxyPage>(), nullptr);

        void PropagateParamModel(YtFlowApp::EditProxyPageParam const &param);

        hstring m_proxyName;
        Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProxyLegModel> m_proxyLegs;
        bool m_isUdpSupported{};
        bool m_isReadonly{};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProxyPage : EditProxyPageT<EditProxyPage, implementation::EditProxyPage>
    {
    };
}
