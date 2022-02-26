#pragma once

#include "NetifHomeWidget.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct NetifHomeWidget : NetifHomeWidgetT<NetifHomeWidget>
    {
        struct Netif
        {
            std::string name;
            std::optional<std::pair<std::array<uint8_t, 4>, uint16_t>> ipv4_addr;
            std::optional<std::pair<std::array<uint8_t, 16>, uint16_t>> ipv6_addr;
            std::vector<std::string> dns_servers;
        };
        struct NetifInfo
        {
            NetifInfo() = default;
            NetifInfo(NetifInfo &&) = default;
            NetifInfo(NetifInfo const &) = default;
            Netif netif;
        };

        NetifHomeWidget();
        NetifHomeWidget(hstring pluginName, std::shared_ptr<std::vector<uint8_t>> sharedInfo);

        void UpdateInfo();

      private:
        std::shared_ptr<std::vector<uint8_t>> m_sharedInfo;
    };

    inline void from_json(nlohmann::json const &json, NetifHomeWidget::Netif &r)
    {
        json.at("name").get_to(r.name);
        auto const v4Doc{json.at("ipv4_addr")};
        auto const v6Doc{json.at("ipv6_addr")};
        if (!v4Doc.is_null())
        {
            std::pair<std::array<uint8_t, 4>, uint16_t> v4Repr = v4Doc;
            r.ipv4_addr = v4Repr;
        }
        if (!v6Doc.is_null())
        {
            std::pair<std::array<uint8_t, 16>, uint16_t> v6Repr = v6Doc;
            r.ipv6_addr = v6Repr;
        }
        json.at("dns_servers").get_to(r.dns_servers);
    }
    inline void to_json(nlohmann::json &, NetifHomeWidget::Netif const &)
    {
        throw hresult_not_implemented{};
    }
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetifHomeWidget::NetifInfo, netif);
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct NetifHomeWidget : NetifHomeWidgetT<NetifHomeWidget, implementation::NetifHomeWidget>
    {
    };
}
