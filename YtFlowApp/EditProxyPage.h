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
        void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args);
        void AdaptiveWidth_StateChanged(Windows::Foundation::IInspectable const &sender,
                                        Windows::UI::Xaml::VisualStateChangedEventArgs const &e);
        fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                         winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void LegList_ItemClick(winrt::Windows::Foundation::IInspectable const &sender,
                               winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs const &e);
        void LegItemDelete_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                 winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ChainBeforeButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                     winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void ChainAfterButton_Click(winrt::Windows::Foundation::IInspectable const &sender,
                                    winrt::Windows::UI::Xaml::RoutedEventArgs const &e);
        void SniAutoSuggestBox_TextChanged(
            winrt::Windows::UI::Xaml::Controls::AutoSuggestBox const &sender,
            winrt::Windows::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const &args);
        void AlpnAutoSuggestBox_TextChanged(
            winrt::Windows::UI::Xaml::Controls::AutoSuggestBox const &sender,
            winrt::Windows::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const &args);
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

        bool PropagateParamModel(YtFlowApp::EditProxyPageParam const &param);

        YtFlowApp::ProxyModel m_proxyModel = nullptr;
        hstring m_proxyName;
        Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProxyLegModel> m_proxyLegs;
        bool m_isUdpSupported{};
        bool m_isReadonly{};
        bool m_forceQuit{};
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProxyPage : EditProxyPageT<EditProxyPage, implementation::EditProxyPage>
    {
    };
}
