#include "pch.h"
#include "NewProfilePage.h"
#if __has_include("NewProfilePage.g.cpp")
#include "NewProfilePage.g.cpp"
#endif

#include "CoreFfi.h"
#include "RawEditorParam.h"

using namespace nlohmann;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    NewProfilePage::NewProfilePage()
    {
        InitializeComponent();
    }

    void NewProfilePage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args)
    {
        bool isWelcoming = args.Parameter().try_as<bool>().value_or(false);
        HeaderControl().Visibility(isWelcoming ? Visibility::Collapsed : Visibility::Visible);
        WelcomeHeaderControl().Visibility(isWelcoming ? Visibility::Visible : Visibility::Collapsed);
    }

    void NewProfilePage::OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs const &args)
    {
        if ((!SaveButton().IsEnabled() || WelcomeHeaderControl().Visibility() == Visibility::Visible) &&
            NewProfileNameText().IsEnabled())
        {
            args.Cancel(true);
            return;
        }
    }

    fire_and_forget NewProfilePage::SaveButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        const auto newProfileName{to_string(NewProfileNameText().Text())};
        if (newProfileName.empty())
        {
            NewProfileNameText().Focus(FocusState::Programmatic);
            co_return;
        }

        SaveButton().IsEnabled(false);
        const auto lifetime{get_strong()};
        bool nameDuplicated{false};
        NewProfileConfig config;
        config.OutboundType = m_selectedOutboundType;

        co_await resume_background();

        auto conn{FfiDbInstance.Connect()};
        for (const auto &profile : conn.GetProfiles())
        {
            if (profile.name == newProfileName)
            {
                nameDuplicated = true;
                break;
            }
        }
        if (!nameDuplicated)
        {
            auto const id{conn.CreateProfile(newProfileName.data(), "en-US")};
            CreatePresetPlugins(id, std::move(config));
        }

        co_await resume_foreground(Dispatcher());
        if (nameDuplicated)
        {
            NewProfileNameText().IsEnabled(true);
            NewProfileNameText().Foreground(Media::SolidColorBrush{Windows::UI::Colors::Red()});
            SaveButton().IsEnabled(true);
        }
        else
        {
            NewProfileNameText().IsEnabled(false);
            Frame().GoBack();
        }
    }

    void NewProfilePage::OutboundTypeButton_Checked(IInspectable const &sender, RoutedEventArgs const & /* e */)
    {
        auto const btn = sender.as<RadioButton>();
        m_selectedOutboundType = btn.Tag().as<hstring>();
    }

    void NewProfilePage::Page_Loaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        SsButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        TrojanButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        HttpButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
    }

    void NewProfilePage::Page_Unloaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        SsButton().Checked(m_ssCheckedToken);
        TrojanButton().Checked(m_trojanCheckedToken);
        HttpButton().Checked(m_httpCheckedToken);
    }

    void NewProfilePage::NewProfileNameText_TextChanged(IInspectable const & /* sender */,
                                                        TextChangedEventArgs const & /* e */)
    {
        NewProfileNameText().Foreground({nullptr});
    }

    void NewProfilePage::CreatePresetPlugins(uint32_t profileId, NewProfileConfig config)
    {
        auto doc{
            "{"
            "  \"entry-ip-stack\": {"
            "    \"desc\": \"Handle TCP or UDP connections from UWP VPN.\","
            "    \"plugin\": \"ip-stack\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"tcp_next\": \"fakeip-dns-server.tcp_map_back.main-forward.tcp\","
            "      \"udp_next\": \"dns-dispatcher.udp\","
            "      \"tun\": \"uwp-vpn-tun.tun\""
            "    }"
            "  },"
            "  \"uwp-vpn-tun\": {"
            "    \"desc\": \"UWP VPN Plugin TUN interface.\","
            "    \"plugin\": \"vpn-tun\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"dns\": [\"11.16.1.1\"],"
            "      \"ipv4\": \"192.168.3.1\","
            "      \"ipv4_route\": ["
            "        \"11.17.0.0/16\","
            "        \"11.16.0.0/16\""
            "      ],"
            "      \"ipv6\": null,"
            "      \"ipv6_route\": [],"
            "      \"web_proxy\": null"
            "    }"
            "  },"
            "  \"main-forward\": {"
            "    \"desc\": \"Forward connections to the main outbound.\","
            "    \"plugin\": \"forward\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"request_timeout\": 200,"
            "      \"tcp_next\": \"ss-client.tcp\","
            "      \"udp_next\": \"phy.udp\""
            "    }"
            "  },"
            "  \"ss-client\": {"
            "    \"desc\": \"Shadowsocks client.\","
            "    \"plugin\": \"shadowsocks-client\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"method\": \"aes-128-gcm\","
            "      \"password\": {"
            "        \"__byte_repr\": \"utf8\","
            "        \"data\": \"my_ss_password\""
            "      },"
            "      \"tcp_next\": \"proxy-redir.tcp\","
            "      \"udp_next\": \"null.udp\""
            "    }"
            "  },"
            "  \"http-proxy-client\": {"
            "    \"desc\": \"HTTP Proxy client. Use HTTP CONNECT to connect to the proxy server.\","
            "    \"plugin\": \"http-proxy-client\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"tcp_next\": \"proxy-redir.tcp\","
            "      \"user\": {"
            "        \"__byte_repr\": \"utf8\","
            "        \"data\": \"\""
            "      },"
            "      \"pass\": {"
            "        \"__byte_repr\": \"utf8\","
            "        \"data\": \"\""
            "      }"
            "    }"
            "  },"
            "  \"trojan-client\": {"
            "    \"desc\": \"Trojan client. The TLS part is in plugin trojan-client-tls.\","
            "    \"plugin\": \"trojan-client\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"password\": {"
            "        \"__byte_repr\": \"utf8\","
            "        \"data\": \"my_trojan_password\""
            "      },"
            "      \"tls_next\": \"trojan-client-tls.tcp\""
            "    }"
            "  },"
            "  \"trojan-client-tls\": {"
            "    \"desc\": \"TLS client stream for Trojan client.\","
            "    \"plugin\": \"tls-client\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"next\": \"proxy-redir.tcp\","
            "      \"skip_cert_check\": false,"
            "      \"sni\": \"my.trojan.proxy.server.com\""
            "    }"
            "  },"
            "  \"proxy-redir\": {"
            "    \"desc\": \"Change the destination to the proxy server.\","
            "    \"plugin\": \"redirect\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"dest\": {"
            "        \"host\": \"my.proxy.server.com.\","
            "        \"port\": 8388"
            "      },"
            "      \"tcp_next\": \"phy.tcp\","
            "      \"udp_next\": \"phy.udp\""
            "    }"
            "  },"
            "  \"phy\": {"
            "    \"desc\": \"The physical NIC.\","
            "    \"plugin\": \"netif\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"family_preference\": \"Ipv4Only\","
            "      \"type\": \"Auto\""
            "    }"
            "  },"
            "  \"null\": {"
            "    \"desc\": \"Return an error for any incoming requests.\","
            "    \"plugin\": \"null\","
            "    \"plugin_version\": 0,"
            "    \"param\": null"
            "  },"
            "  \"reject\": {"
            "    \"desc\": \"Silently drop any outgoing requests.\","
            "    \"plugin\": \"reject\","
            "    \"plugin_version\": 0,"
            "    \"param\": null"
            "  },"
            "  \"fake-ip\": {"
            "    \"desc\": \"Assign a fake IP address for each domain name. This is useful for TUN inbounds where "
            "incoming connections carry no information about domain names. By using a Fake IP resolver, destination IP "
            "addresses can be mapped back to a domain name that the client is connecting to.\","
            "    \"plugin\": \"fake-ip\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"fallback\": \"null.resolver\","
            "      \"prefix_v4\": [11, 17],"
            "      \"prefix_v6\": [38, 12, 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]"
            "    }"
            "  },"
            "  \"dns-dispatcher\": {"
            "    \"desc\": \"Dispatches DNS requests to our DNS server.\","
            "    \"plugin\": \"simple-dispatcher\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"fallback_tcp\": \"main-forward.tcp\","
            "      \"fallback_udp\": \"main-forward.udp\","
            "      \"rules\": ["
            "        {"
            "          \"src\": {"
            "            \"ip_ranges\": [\"0.0.0.0/0\"],"
            "            \"port_ranges\": [{ \"start\": 0, \"end\": 65535 }]"
            "          },"
            "          \"dst\": {"
            "            \"ip_ranges\": [\"11.16.1.1/32\"],"
            "            \"port_ranges\": [ { \"start\": 53, \"end\": 53 }]"
            "          },"
            "          \"is_udp\": true,"
            "          \"next\": \"fakeip-dns-server.udp\""
            "        }"
            "      ]"
            "    }"
            "  },"
            "  \"fakeip-dns-server\": {"
            "    \"desc\": \"Respond to DNS request messages using results from the FakeIP resolver.\","
            "    \"plugin\": \"dns-server\","
            "    \"plugin_version\": 0,"
            "    \"param\": {"
            "      \"concurrency_limit\": 64,"
            "      \"resolver\": \"fake-ip.resolver\","
            "      \"tcp_map_back\": [\"main-forward.tcp\"],"
            "      \"udp_map_back\": [\"main-forward.udp\"],"
            "      \"ttl\": 60"
            "    }"
            "  }"
            "}"_json};

        if (config.OutboundType == L"http")
        {
            doc["main-forward"]["param"]["tcp_next"] = "http-proxy-client.tcp";
        }
        else if (config.OutboundType == L"trojan")
        {
            doc["main-forward"]["param"]["tcp_next"] = "trojan-client.tcp";
        }

        auto conn{FfiDbInstance.Connect()};
        for (auto const &[pluginName, pluginDoc] : doc.items())
        {
            std::string const &desc = pluginDoc["desc"];
            std::string const &plugin = pluginDoc["plugin"];
            uint16_t const pluginVersion = pluginDoc["plugin_version"];
            auto &paramDoc = pluginDoc["param"];
            RawEditorParam::UnescapeCborBuf(paramDoc);
            auto const param{json::to_cbor(paramDoc)};
            auto const id = conn.CreatePlugin(profileId, pluginName.data(), desc.data(), plugin.data(), pluginVersion,
                                              param.data(), param.size());
            if (pluginName == "entry-ip-stack")
            {
                conn.SetPluginAsEntry(id, profileId);
            }
        }
    }
}
