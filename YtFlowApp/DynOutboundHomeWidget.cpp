#include "pch.h"
#include "DynOutboundHomeWidget.h"
#if __has_include("DynOutboundHomeWidget.g.cpp")
#include "DynOutboundHomeWidget.g.cpp"
#endif

#include "DynOutboundProxyModel.h"
#include "UI.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    struct DynOutboundInfo
    {
        std::string current_proxy_name;
        uint32_t current_proxy_idx;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DynOutboundInfo, current_proxy_name, current_proxy_idx)
    struct DynOutboundProxy
    {
        std::string name;
        uint32_t idx;
        uint32_t id;
        uint32_t group_id;
        std::string group_name;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DynOutboundProxy, name, idx, id, group_id, group_name)
    struct DynOutboundListProxiesRes
    {
        std::vector<DynOutboundProxy> proxies;
        // TODO: fixed outbounds
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DynOutboundListProxiesRes, proxies)

    DynOutboundHomeWidget::DynOutboundHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo,
                                                 RequestSender sendRequest)
        : m_pluginName(std::move(pluginName)), m_sharedInfo(std::move(sharedInfo)),
          m_sendRequest(std::move(sendRequest))
    {
    }

    void DynOutboundHomeWidget::UpdateInfo()
    {
        hstring proxyName;
        try
        {
            DynOutboundInfo const info = nlohmann::json::from_cbor(*m_sharedInfo);
            proxyName = to_hstring(info.current_proxy_name);
        }
        catch (nlohmann::json::type_error)
        {
        }
        if (ProxyNameText().Text() != proxyName)
        {
            PreviewProxyNameText().Text(proxyName);
            ProxyNameText().Text(std::move(proxyName));
        }
    }

    void DynOutboundHomeWidget::UserControl_Loaded(IInspectable const &, RoutedEventArgs const &)
    {
        PluginNameText().Text(m_pluginName);
    }

    fire_and_forget DynOutboundHomeWidget::SelectProxyButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        auto const lifetime = get_strong();
        try
        {
            SelectProxyButton().IsEnabled(false);

            auto const res = co_await lifetime->m_sendRequest("list_proxies", nlohmann::json::to_cbor(nullptr));
            const auto [proxies] = nlohmann::json::from_cbor(res).get<DynOutboundListProxiesRes>();
            std::vector<YtFlowApp::DynOutboundProxyModel> models;
            models.reserve(proxies.size());
            std::ranges::transform(proxies, std::back_inserter(models), [](auto const &item) {
                return make<DynOutboundProxyModel>(item.idx, to_hstring(item.name), to_hstring(item.group_name));
            });
            co_await resume_foreground(lifetime->Dispatcher());

            ProxyItemGridView().ItemsSource(nullptr);
            ProxyItemGridView().ItemsSource(single_threaded_observable_vector(std::move(models)));
            VisualStateManager::GoToState(*this, L"DisplayProxySelectionView", true);
            co_return;
        }
        catch (...)
        {
            NotifyException(L"Loading proxies for selection");
        }
        co_await resume_foreground(lifetime->Dispatcher());
        SelectProxyButton().IsEnabled(true);
    }

    void DynOutboundHomeWidget::ProxySelectionBackButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        VisualStateManager::GoToState(*this, L"DisplayInfoView", true);
        SelectProxyButton().IsEnabled(true);
    }

    fire_and_forget DynOutboundHomeWidget::ProxyItem_Click(IInspectable const &sender, RoutedEventArgs const &)
    {
        try
        {
            auto const lifetime = get_strong();
            auto const proxyIdx = sender.as<FrameworkElement>().DataContext().as<DynOutboundProxyModel>()->Idx();
            auto const res = co_await lifetime->m_sendRequest("select", nlohmann::json::to_cbor(proxyIdx));
            if (auto const doc = nlohmann::json::from_cbor(res); doc.is_string())
            {
                NotifyUser(to_hstring(doc.get<std::string_view>()), L"Failed to select proxy");
                co_return;
            }

            co_await resume_foreground(lifetime->Dispatcher());
            VisualStateManager::GoToState(*lifetime, L"DisplayInfoView", true);
            lifetime->SelectProxyButton().IsEnabled(true);
        }
        catch (...)
        {
            NotifyException(L"Selecting proxy");
        }
    }
}
