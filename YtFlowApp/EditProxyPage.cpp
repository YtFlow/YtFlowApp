#include "pch.h"
#include "EditProxyPage.h"
#if __has_include("EditProxyPage.g.cpp")
#include "EditProxyPage.g.cpp"
#endif

#include "EditProxyPageParam.h"
#include "ProxyLegModel.h"
#include "ProxyModel.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Navigation;
using Windows::UI::Core::CoreDispatcherPriority;

namespace winrt::YtFlowApp::implementation
{
    EditProxyPage::EditProxyPage()
    {
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
        VisualStateManager::GoToState(*this, L"MasterState", false);
        bool analyzed = true;
        if (auto const param = args.Parameter().try_as<EditProxyPageParam>(); param)
        {
            m_proxyModel = *param->Proxy;
            analyzed = PropagateParamModel(*param);
        }
        if (!analyzed)
        {
            NotifyUser(hstring(L"The proxy is too complex to be parsed. While a dyn-outbound plugin may pick up and "
                               L"load this proxy at runtime, it is not possible to edit it. Any changes saved will "
                               L"overwrite the existing proxy configuration."),
                       L"Proxy too complex");
        }
        co_return;
    }
    fire_and_forget EditProxyPage::OnNavigatingFrom(
        Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args)
    {
        try
        {
            auto const currState{AdaptiveWidthVisualStateGroup().CurrentState()};
            if (currState != nullptr && currState.Name() == L"DetailState")
            {
                VisualStateManager::GoToState(*this, L"MasterState", true);
                LegList().SelectedIndex(-1);
                args.Cancel(true);
                co_return;
            }
            auto const navArgs{args};
            auto const lifetime{get_strong()};

            if (std::exchange(m_forceQuit, false))
            {
                co_return;
            }

            if (!IsDirty())
            {
                co_return;
            }

            navArgs.Cancel(true);
            if (co_await QuitWithUnsavedDialog().ShowAsync() != ContentDialogResult::Primary)
            {
                co_return;
            }

            m_forceQuit = true;
            switch (navArgs.NavigationMode())
            {
            case NavigationMode::Back:
                Frame().GoBack();
                break;
            case NavigationMode::Forward:
                Frame().GoForward();
                break;
            default:
                Frame().Navigate(navArgs.SourcePageType(), navArgs.NavigationTransitionInfo());
                break;
            }
        }
        catch (...)
        {
            NotifyException(L"EditProxyPage OnNavigatingFrom");
        }
    }
    void EditProxyPage::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const &)
    {
        m_proxyModel = nullptr;
        m_proxyLegs = nullptr;
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
    bool EditProxyPage::PropagateParamModel(YtFlowApp::EditProxyPageParam const &paramModel)
    {
        auto const param = get_self<EditProxyPageParam>(paramModel);
        m_isReadonly = param->IsReadonly;
        auto const &analyzeResult = param->Proxy->Analyze();
        FfiProxy dummyProxy = {.name = "", .legs = {}, .udp_supported = false};
        FfiProxy const *analyzed = &dummyProxy;
        if (analyzeResult.has_value())
        {
            analyzed = &analyzeResult.value();
        }
        else
        {
            dummyProxy.name = to_string(param->Proxy->Name());
            dummyProxy.legs.emplace_back(FfiProxyLeg{});
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
        m_proxyLegs.VectorChanged([weak](auto const &, auto const &) {
            if (auto self{weak.get()})
            {
                self->IsDirty(true);
            }
        });
        IsDirty(false);
        Bindings->Update();

        LegList().SelectedIndex(0);
        return analyzeResult.has_value();
    }

    fire_and_forget EditProxyPage::SaveButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        using std::ranges::to;
        using std::ranges::views::transform;

        try
        {
            auto const lifetime = get_strong();
            auto const proxyModelLifetime = m_proxyModel;
            if (!proxyModelLifetime)
            {
                co_return;
            }
            for (auto const &proxyLegModel : m_proxyLegs)
            {
                auto const proxyLeg = get_self<ProxyLegModel>(proxyLegModel);
                if (proxyLeg->Host().empty())
                {
                    LegList().SelectedItem(proxyLegModel);
                    NotifyUser(L"Host cannot be empty.", L"Invalid Proxy");
                    co_return;
                }
                if (proxyLeg->ProtocolType() == L"VMess")
                {
                    GUID userGuid{};
                    hstring const guidStr = hstring(L"{" + proxyLeg->Password() + L"}");
                    if (FAILED(CLSIDFromString(guidStr.c_str(), &userGuid)))
                    {
                        LegList().SelectedItem(proxyLegModel);
                        NotifyUser(L"Invalid VMess UUID.", L"Invalid Proxy");
                        co_return;
                    }
                }
            }
            auto const proxyModel = get_self<ProxyModel>(proxyModelLifetime);
            FfiProxy proxy = {.name = to_string(m_proxyName),
                              .legs = m_proxyLegs | transform([](auto const &legModel) {
                                          return get_self<ProxyLegModel>(legModel)->Encode();
                                      }) |
                                      to<std::vector>(),
                              .udp_supported = m_isUdpSupported};

            auto const proxyBuf = nlohmann::json::to_cbor(proxy);
            auto const proxyDataBuf = unwrap_ffi_byte_buffer(
                ytflow_core::ytflow_app_proxy_data_proxy_compose_v1(proxyBuf.data(), proxyBuf.size()));
            proxyModel->Name(m_proxyName);
            proxyModel->Proxy(proxyDataBuf);
            proxyModel->ProxyVersion(0);

            co_await resume_background();
            proxyModel->Update();
            co_await resume_foreground(lifetime->Dispatcher());
            if (m_proxyModel == proxyModelLifetime)
            {
                IsDirty(false);
            }
        }
        catch (...)
        {
            NotifyException(L"EditProxyPage Save");
        }
    }

    void EditProxyPage::LegList_ItemClick(IInspectable const &, ItemClickEventArgs const &e)
    {
        try
        {
            auto const lifetime = get_strong();
            auto const clickedItem = e.ClickedItem().try_as<YtFlowApp::ProxyLegModel>();
            if (!clickedItem)
            {
                return;
            }
            uint32_t clickedIndex{};
            if (!m_proxyLegs.IndexOf(clickedItem, clickedIndex))
            {
                return;
            }
            LegList().SelectedIndex(clickedIndex);
            VisualStateManager::GoToState(*this, L"DetailState", true);
        }
        catch (...)
        {
            NotifyException(L"EditProxyPage LegList_ItemClick");
        }
    }

    void EditProxyPage::LegItemDelete_Click(IInspectable const &sender, RoutedEventArgs const &)
    {
        if (m_proxyLegs.Size() == 1)
        {
            NotifyUser(L"Cannot delete the last proxy leg.", L"Invalid Proxy");
            return;
        }

        auto const legModel = sender.as<FrameworkElement>().DataContext().try_as<YtFlowApp::ProxyLegModel>();
        if (!legModel)
        {
            return;
        }

        uint32_t index{};
        if (!m_proxyLegs.IndexOf(legModel, index))
        {
            return;
        }
        m_proxyLegs.RemoveAt(index);
        IsDirty(true);
    }

    void EditProxyPage::ChainBeforeButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        auto selectedIndex = LegList().SelectedIndex();
        if (selectedIndex == -1)
        {
            selectedIndex = 0;
        }
        m_proxyLegs.InsertAt(selectedIndex, make<ProxyLegModel>());
        IsDirty(true);
    }
    void EditProxyPage::ChainAfterButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        auto selectedIndex = LegList().SelectedIndex();
        if (selectedIndex == -1)
        {
            selectedIndex = LegList().Items().Size() - 1;
        }
        m_proxyLegs.InsertAt(selectedIndex + 1, make<ProxyLegModel>());
        IsDirty(true);
    }

    void EditProxyPage::SniAutoSuggestBox_TextChanged(AutoSuggestBox const &sender,
                                                      AutoSuggestBoxTextChangedEventArgs const &args)
    {
        if (args.Reason() != AutoSuggestionBoxTextChangeReason::UserInput)
        {
            return;
        }

        using namespace std::string_view_literals;
        sender.Items().Clear();
        auto const text = sender.Text();
        if (L"auto"sv.starts_with(text) && text != L"auto")
        {
            sender.Items().Append(box_value(hstring{L"auto"}));
        }
    }

    void EditProxyPage::AlpnAutoSuggestBox_TextChanged(AutoSuggestBox const &sender,
                                                       AutoSuggestBoxTextChangedEventArgs const &args)
    {
        if (args.Reason() != AutoSuggestionBoxTextChangeReason::UserInput)
        {
            return;
        }

        using namespace std::string_view_literals;
        sender.Items().Clear();
        auto const text = sender.Text();
        if (L"auto"sv.starts_with(text) && text != L"auto")
        {
            sender.Items().Append(box_value(hstring{L"auto"}));
        }
        if (L"http/1.1"sv.starts_with(text) && text != L"http/1.1")
        {
            sender.Items().Append(box_value(hstring{L"http/1.1"}));
        }
    }
}
