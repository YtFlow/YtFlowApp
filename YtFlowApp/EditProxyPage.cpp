#include "pch.h"
#include "EditProxyPage.h"
#if __has_include("EditProxyPage.g.cpp")
#include "EditProxyPage.g.cpp"
#endif

#include "EditProxyPageParam.h"
#include "ProxyLegModel.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    EditProxyPage::EditProxyPage()
    {
        VisualStateManager::GoToState(*this, L"MasterState", false);
    }
    Windows::UI::Xaml::DependencyProperty EditProxyPage::IsDirtyProperty()
    {
        return m_isDirtyProperty;
    }
    Visibility EditProxyPage::IsShadowsocks(hstring const &protocolType) noexcept
    {
        return protocolType == L"Shadowsocks" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsTrojan(hstring const &protocolType) noexcept
    {
        return protocolType == L"Trojan" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsHttp(hstring const &protocolType) noexcept
    {
        return protocolType == L"HTTP" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsSocks5(hstring const &protocolType) noexcept
    {
        return protocolType == L"SOCKS5" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsVMess(hstring const &protocolType) noexcept
    {
        return protocolType == L"VMess" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsHttpObfs(hstring const &obfsType) noexcept
    {
        return obfsType == L"simple-obfs (HTTP)" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsTlsObfs(hstring const &obfsType) noexcept
    {
        return obfsType == L"simple-obfs (TLS)" ? Visibility::Visible : Visibility::Collapsed;
    }
    Visibility EditProxyPage::IsWebSocket(hstring const &obfsType) noexcept
    {
        return obfsType == L"WebSocket" ? Visibility::Visible : Visibility::Collapsed;
    }
    bool EditProxyPage::TlsParamsAllowed(bool enableTls, bool isReadonly) noexcept
    {
        return enableTls && !isReadonly;
    }
    fire_and_forget EditProxyPage::OnNavigatedTo(Navigation::NavigationEventArgs const &args)
    {
        auto const lifetime = get_strong();
        if (auto const param = args.Parameter().try_as<EditProxyPageParam>(); param)
        {
            PropagateParamModel(*param);
        }
        co_return;
    }
    fire_and_forget EditProxyPage::OnNavigatingFrom(
        Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args)
    {
        auto const lifetime = get_strong();
        args.Cancel(false);
        co_return;
    }
    void EditProxyPage::AdaptiveWidth_StateChanged(IInspectable const & /* sender */,
                                                   VisualStateChangedEventArgs const &e)
    {
        try
        {
            auto const newState{e.NewState()};
            if (newState != nullptr || newState == MediumWidthState())
            {
                return;
            }

            if (LegList().SelectedIndex() == -1)
            {
                VisualStateManager::GoToState(*this, L"MasterState", true);
            }
            else
            {
                VisualStateManager::GoToState(*this, L"DetailState", true);
            }
        }
        catch (...)
        {
            NotifyException(L"EditProxyPage AdaptiveWidth StateChange");
        }
    }
    hstring EditProxyPage::ProxyName()
    {
        return m_proxyName;
    }
    void EditProxyPage::ProxyName(hstring proxyName)
    {
        if (m_proxyName != proxyName)
        {
            m_proxyName = proxyName;
            IsDirty(true);
        }
    }
    Windows::Foundation::Collections::IObservableVector<YtFlowApp::ProxyLegModel> EditProxyPage::ProxyLegs()
    {
        return m_proxyLegs;
    }
    bool EditProxyPage::IsUdpSupported() const
    {
        return m_isUdpSupported;
    }
    void EditProxyPage::IsUdpSupported(bool isUdpSupported)
    {
        if (m_isUdpSupported != isUdpSupported)
        {
            m_isUdpSupported = isUdpSupported;
            IsDirty(true);
        }
    }
    bool EditProxyPage::IsReadonly() const
    {
        return m_isReadonly;
    }
    bool EditProxyPage::IsWritable() const
    {
        return !m_isReadonly;
    }
    bool EditProxyPage::IsDirty() const
    {
        return GetValue(m_isDirtyProperty).try_as<bool>().value_or(false);
    }
    void EditProxyPage::IsDirty(bool isDirty) const
    {
        if (IsDirty() != isDirty)
        {
            SetValue(m_isDirtyProperty, box_value(isDirty));
        }
    }
    void EditProxyPage::PropagateParamModel(YtFlowApp::EditProxyPageParam const &paramModel)
    {
        auto const param = get_self<EditProxyPageParam>(paramModel);
        m_isReadonly = param->IsReadonly;
        auto const &analyzeResult = param->Proxy->Analyze();
        static FfiProxy dummyProxy = {.name = "", .legs = {}, .udp_supported = false};
        FfiProxy const *analyzed = &dummyProxy;
        if (analyzeResult.has_value())
        {
            analyzed = &analyzeResult.value();
        }

        m_proxyName = to_hstring(analyzed->name);
        m_isUdpSupported = analyzed->udp_supported;
        using std::ranges::to;
        using std::ranges::views::transform;
        auto const weak = get_weak();
        m_proxyLegs = single_threaded_observable_vector(analyzed->legs |
                                                        transform([&weak, isReadonly = m_isReadonly](auto const &leg) {
                                                            auto model = make_self<ProxyLegModel>(isReadonly, leg);
                                                            model->PropertyChanged([weak](auto const &, auto const &) {
                                                                if (auto self{weak.get()})
                                                                {
                                                                    self->IsDirty(true);
                                                                }
                                                            });
                                                            return YtFlowApp::ProxyLegModel(*model);
                                                        }) |
                                                        to<std::vector>());
        IsDirty(false);

        Bindings->Update();
        LegList().SelectedIndex(0);
    }
}
