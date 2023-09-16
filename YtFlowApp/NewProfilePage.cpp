#include "pch.h"
#include "NewProfilePage.h"
#if __has_include("NewProfilePage.g.cpp")
#include "NewProfilePage.g.cpp"
#endif

#include "CoreFfi.h"
#include "RawEditorParam.h"
#include "SplitRoutingRulesetControl.h"
#include "UI.h"

using namespace nlohmann;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;

namespace winrt::YtFlowApp::implementation
{
    NewProfilePage::NewProfilePage()
    {
        InitializeComponent();
    }

    void NewProfilePage::OnNavigatedTo(Navigation::NavigationEventArgs const &args)
    {
        bool const isWelcoming = args.Parameter().try_as<bool>().value_or(false);
        HeaderControl().Visibility(isWelcoming ? Visibility::Collapsed : Visibility::Visible);
        WelcomeHeaderControl().Visibility(isWelcoming ? Visibility::Visible : Visibility::Collapsed);

        hstring suggestedName;
        try
        {
            auto conn{FfiDbInstance.Connect()};
            suggestedName = hstring{L"New Profile "} + to_hstring(conn.GetProfiles().size() + 1);
        }
        catch (...)
        {
        }
        NewProfileNameText().Text(suggestedName);
    }

    void NewProfilePage::OnNavigatingFrom(Navigation::NavigatingCancelEventArgs const &args)
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
        try
        {
            auto const newProfileName{to_string(NewProfileNameText().Text())};
            if (newProfileName.empty())
            {
                NewProfileNameText().Focus(FocusState::Programmatic);
                co_return;
            }

            auto selectedSplitRoutingType =
                SplitRoutingModeButtons().SelectedItem().as<RadioButton>().Tag().as<hstring>();
            if (selectedSplitRoutingType != L"all")
            {
                if (SelectedRulesetNameText().Text() == L"")
                {
                    SelectRulesetButton_Click(nullptr, nullptr);
                    co_return;
                }
            }

            SaveButton().IsEnabled(false);
            const auto lifetime{get_strong()};
            bool nameDuplicated{false};
            NewProfileConfig config;
            config.InboundMode = InboundModeButtons().SelectedItem().as<RadioButton>().Tag().as<hstring>();
            config.OutboundType = m_selectedOutboundType;
            config.SplitRoutingMode = std::move(selectedSplitRoutingType);
            config.SplitRoutingRuleset = SelectedRulesetNameText().Text();
            config.RuleResolver = RuleResolverComboBox().SelectedItem().as<ComboBoxItem>().Tag().as<hstring>();

            co_await resume_background();

            auto conn{FfiDbInstance.Connect()};
            for (auto const &[_id, name, _locale] : conn.GetProfiles())
            {
                if (name == newProfileName)
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
        catch (...)
        {
            NotifyException(L"Saving");
        }
    }

    void NewProfilePage::OutboundTypeButton_Checked(IInspectable const &sender, RoutedEventArgs const & /* e */)
    {
        auto const btn = sender.as<FrameworkElement>();
        m_selectedOutboundType = btn.Tag().as<hstring>();
        if (m_selectedOutboundType == L"dyn")
        {
            DynOutboundDisabledText().Visibility(Visibility::Collapsed);
            DynOutboundEnabledText().Visibility(Visibility::Visible);
            SsButton().IsChecked(false);
            TrojanButton().IsChecked(false);
            VmessWsTlsButton().IsChecked(false);
            HttpButton().IsChecked(false);
            SsButton().IsEnabled(false);
            TrojanButton().IsEnabled(false);
            VmessWsTlsButton().IsEnabled(false);
            HttpButton().IsEnabled(false);
        }
    }
    void NewProfilePage::SplitRoutingModeButtons_SelectionChanged(IInspectable const &,
                                                                  SelectionChangedEventArgs const &e)
    {
        auto const selectedIt = e.AddedItems().First();
        if (!selectedIt.HasCurrent())
        {
            return;
        }
        auto const selected = selectedIt.Current().as<FrameworkElement>();
        if (!selected)
        {
            return;
        }
        bool const splitRoutingEnabled = selected.Tag().as<hstring>() != L"all";
        SelectRulesetButton().IsEnabled(splitRoutingEnabled);
        RuleResolverComboBox().IsEnabled(splitRoutingEnabled);
    }
    void NewProfilePage::DynOutboundButton_Unchecked(IInspectable const &, RoutedEventArgs const &)
    {
        DynOutboundDisabledText().Visibility(Visibility::Visible);
        DynOutboundEnabledText().Visibility(Visibility::Collapsed);
        SsButton().IsEnabled(true);
        SsButton().IsChecked(true);
        TrojanButton().IsEnabled(true);
        VmessWsTlsButton().IsEnabled(true);
        HttpButton().IsEnabled(true);
    }

    void NewProfilePage::Page_Loaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        m_dynOutboundCheckedToken = DynOutboundButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        m_ssCheckedToken = SsButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        m_trojanCheckedToken = TrojanButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        m_vmessWsTlsCheckedToken = VmessWsTlsButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        m_httpCheckedToken = HttpButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
    }

    void NewProfilePage::Page_Unloaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        DynOutboundButton().Checked(m_dynOutboundCheckedToken);
        SsButton().Checked(m_ssCheckedToken);
        TrojanButton().Checked(m_trojanCheckedToken);
        VmessWsTlsButton().Checked(m_vmessWsTlsCheckedToken);
        HttpButton().Checked(m_httpCheckedToken);
    }

    void NewProfilePage::NewProfileNameText_TextChanged(IInspectable const & /* sender */,
                                                        TextChangedEventArgs const & /* e */)
    {
        NewProfileNameText().Foreground({nullptr});
    }

    void NewProfilePage::CreatePresetPlugins(uint32_t profileId, NewProfileConfig config)
    {
        try
        {
            auto doc{
                "{"
                "  \"entry-ip-stack\": {"
                "    \"desc\": \"Handle TCP or UDP connections from UWP VPN.\","
                "    \"plugin\": \"ip-stack\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"tcp_next\": \"fakeip-dns-server.tcp_map_back.geoip-dispatcher.tcp\","
                "      \"udp_next\": \"dns-dispatcher.udp\","
                "      \"tun\": \"uwp-vpn-tun.tun\""
                "    }"
                "  },"
                "  \"uwp-vpn-tun\": {"
                "    \"desc\": \"UWP VPN Plugin TUN interface.\","
                "    \"plugin\": \"vpn-tun\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"dns\": ["
                "        \"11.16.1.1\","
                "        \"260c:1::1\""
                "      ],"
                "      \"ipv4\": \"192.168.3.1\","
                "      \"ipv4_route\": ["
                "        \"11.17.0.0/16\","
                "        \"11.16.0.0/16\","
                "        \"149.154.160.0/20\","
                "        \"91.108.56.130/32\""
                "      ],"
                "      \"ipv6\": \"fd00::2\","
                "      \"ipv6_route\": ["
                "        \"::/16\","
                "        \"260c:2001::/96\","
                "        \"260c:1::/96\","
                "        \"2001:0b28:f23c::/46\","
                "        \"2001:067c:04e8::/48\""
                "      ],"
                "      \"web_proxy\": null"
                "    }"
                "  },"
                "  \"proxy-forward\": {"
                "    \"desc\": \"Forward connections to the proxy outbound.\","
                "    \"plugin\": \"forward\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"request_timeout\": 200,"
                "      \"tcp_next\": \"outbound.tcp\","
                "      \"udp_next\": \"phy.udp\""
                "    }"
                "  },"
                "  \"direct-forward\": {"
                "    \"desc\": \"Forward connections to the physical outbound.\","
                "    \"plugin\": \"forward\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"request_timeout\": 200,"
                "      \"tcp_next\": \"phy.tcp\","
                "      \"udp_next\": \"phy.udp\""
                "    }"
                "  },"
                "  \"outbound\": {"
                "    \"desc\": \"Allows runtime selection of outbound proxies from the Library.\","
                "    \"plugin\": \"dyn-outbound\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"tcp_next\": \"phy.tcp\","
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
                "    \"desc\": \"Trojan client. The TLS part is in plugin client-tls.\","
                "    \"plugin\": \"trojan-client\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"password\": {"
                "        \"__byte_repr\": \"utf8\","
                "        \"data\": \"my_trojan_password\""
                "      },"
                "      \"tls_next\": \"proxy-redir.tcp\""
                "    }"
                "  },"
                "  \"client-tls\": {"
                "    \"desc\": \"TLS client stream for proxy client.\","
                "    \"plugin\": \"tls-client\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"next\": \"phy.tcp\","
                "      \"skip_cert_check\": false,"
                "      \"sni\": \"my.proxy.server.com\""
                "    }"
                "  },"
                "  \"vmess-client\": {"
                "    \"desc\": \"VMess client.\","
                "    \"plugin\": \"vmess-client\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"user_id\": \"b831381d-6324-4d53-ad4f-8cda48b30811\","
                "      \"security\": \"auto\","
                "      \"alter_id\": 0,"
                "      \"tcp_next\": \"proxy-redir.tcp\""
                "    }"
                "  },"
                "  \"ws-client\": {"
                "    \"desc\": \"WebSocket client.\","
                "    \"plugin\": \"ws-client\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"host\": \"dl.microsoft.com\","
                "      \"path\": \"/\","
                "      \"headers\": {},"
                "      \"next\": \"client-tls.tcp\""
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
                "      \"family_preference\": \"Both\","
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
                "incoming connections carry no information about domain names. By using a Fake IP resolver, "
                "destination IP "
                "addresses can be mapped back to a domain name that the client is connecting to.\","
                "    \"plugin\": \"fake-ip\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"fallback\": \"null.resolver\","
                "      \"prefix_v4\": [11, 17],"
                "      \"prefix_v6\": [38, 12, 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]"
                "    }"
                "  },"
                "  \"doh-resolver\": {"
                "    \"desc\": \"Resolves real IP addresses from a secure, trusted provider.\","
                "    \"plugin\": \"host-resolver\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"udp\": [],"
                "      \"tcp\": [],"
                "      \"doh\": [{"
                "        \"url\": \"https://1.1.1.1/dns-query\","
                "        \"next\": \"general-tls.tcp\""
                "      }]"
                "    }"
                "  },"
                "  \"dns-dispatcher\": {"
                "    \"desc\": \"Dispatches DNS requests to our DNS server.\","
                "    \"plugin\": \"simple-dispatcher\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"fallback_tcp\": \"reject.tcp\","
                "      \"fallback_udp\": \"fakeip-dns-server.udp_map_back.geoip-dispatcher.udp\","
                "      \"rules\": ["
                "        {"
                "          \"src\": {"
                "            \"ip_ranges\": [\"0.0.0.0/0\", \"::/0\"],"
                "            \"port_ranges\": [{ \"start\": 0, \"end\": 65535 }]"
                "          },"
                "          \"dst\": {"
                "            \"ip_ranges\": [\"11.16.1.1/32\", \"260c:1::1/128\"],"
                "            \"port_ranges\": [ { \"start\": 53, \"end\": 53 }]"
                "          },"
                "          \"is_udp\": true,"
                "          \"next\": \"fakeip-dns-server.udp\""
                "        }"
                "      ]"
                "    }"
                "  },"
                "  \"geoip-dispatcher\": {"
                "    \"desc\": \"Split routing by GeoIP rules.\","
                "    \"plugin\": \"rule-dispatcher\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"resolver\": \"doh-resolver.resolver\","
                "      \"source\": \"dreamacro-geoip\","
                "      \"actions\": {"
                "        \"direct\": {"
                "          \"tcp\": \"direct-forward.tcp\","
                "          \"udp\": \"direct-forward.udp\""
                "        },"
                "        \"proxy\": {"
                "          \"tcp\": \"proxy-forward.tcp\","
                "          \"udp\": \"proxy-forward.udp\""
                "        }"
                "      },"
                "      \"rules\": {"
                "        \"cn\": \"direct\""
                "      },"
                "      \"fallback\": {"
                "        \"tcp\": \"proxy-forward.tcp\","
                "        \"udp\": \"proxy-forward.udp\""
                "      }"
                "    }"
                "  },"
                "  \"general-tls\": {"
                "    \"desc\": \"TLS client stream for h2, DoH etc.\","
                "    \"plugin\": \"tls-client\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"next\": \"phy.tcp\","
                "      \"skip_cert_check\": false"
                "    }"
                "  },"
                "  \"fakeip-dns-server\": {"
                "    \"desc\": \"Respond to DNS request messages using results from the FakeIP resolver.\","
                "    \"plugin\": \"dns-server\","
                "    \"plugin_version\": 0,"
                "    \"param\": {"
                "      \"concurrency_limit\": 64,"
                "      \"resolver\": \"fake-ip.resolver\","
                "      \"tcp_map_back\": [\"proxy-forward.tcp\"],"
                "      \"udp_map_back\": [\"proxy-forward.udp\"],"
                "      \"ttl\": 60"
                "    }"
                "  }"
                "}"_json};

            if (config.InboundMode == L"full")
            {
                doc["uwp-vpn-tun"]["param"]["ipv4_route"] = {"0.0.0.0/1", "128.0.0.0/1"};
                doc["uwp-vpn-tun"]["param"]["ipv6_route"] = {"::/1", "8000::/1"};
            }

            std::string_view ruleResolver = "phy.resolver";
            if (config.RuleResolver == L"1111")
            {
                ruleResolver = "doh-resolver.resolver";
            }
            else if (config.RuleResolver == L"ali")
            {
                doc["doh-resolver"]["param"]["doh"][0]["url"] = "https://223.5.5.5/dns-query";
                ruleResolver = "doh-resolver.resolver";
            }
            doc["geoip-dispatcher"]["param"]["resolver"] = ruleResolver;

            if (config.SplitRoutingMode == L"all")
            {
                doc["entry-ip-stack"]["param"]["tcp_next"] = "fakeip-dns-server.tcp_map_back.proxy-forward.tcp";
                doc["dns-dispatcher"]["param"]["fallback_udp"] = "fakeip-dns-server.udp_map_back.proxy-forward.udp";
                doc.erase("direct-forward");
            }
            else if (config.SplitRoutingMode == L"whitelist")
            {
                doc["fakeip-dns-server"]["param"]["tcp_map_back"].push_back("geoip-dispatcher.tcp");
                doc["fakeip-dns-server"]["param"]["udp_map_back"].push_back("geoip-dispatcher.udp");
            }
            else if (config.SplitRoutingMode == L"overseas")
            {
                doc["geoip-dispatcher"]["param"]["rules"]["cn"] = "proxy";
                doc["geoip-dispatcher"]["param"]["fallback"]["tcp"] = "direct-forward.tcp";
                doc["geoip-dispatcher"]["param"]["fallback"]["udp"] = "direct-forward.udp";
                doc["fakeip-dns-server"]["param"]["tcp_map_back"].push_back("geoip-dispatcher.tcp");
                doc["fakeip-dns-server"]["param"]["udp_map_back"].push_back("geoip-dispatcher.udp");
            }
            doc["geoip-dispatcher"]["param"]["source"] = to_string(config.SplitRoutingRuleset);

            if (config.OutboundType == L"ss")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "ss-client.tcp";
            }
            else if (config.OutboundType == L"http")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "http-proxy-client.tcp";
            }
            else if (config.OutboundType == L"trojan")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "trojan-client.tcp";
                doc["proxy-redir"]["param"]["tcp_next"] = "client-tls.tcp";
                doc["client-tls"]["param"]["alpn"] = {"h2", "http/1.1"};
            }
            else if (config.OutboundType == L"vmess_ws_tls")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "vmess-client.tcp";
                doc["proxy-redir"]["param"]["tcp_next"] = "ws-client.tcp";
                doc["client-tls"]["param"]["alpn"] = {"http/1.1"};
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
                auto const id = conn.CreatePlugin(profileId, pluginName.data(), desc.data(), plugin.data(),
                                                  pluginVersion, param.data(), param.size());
                if (pluginName == "entry-ip-stack")
                {
                    conn.SetPluginAsEntry(id, profileId);
                }
            }
        }
        catch (...)
        {
            NotifyException(L"Creating preset plugins");
        }
    }

    fire_and_forget NewProfilePage::SelectRulesetButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        co_await RulesetDialog().ShowAsync();
        if (RulesetDialog().RulesetSelected())
        {
            auto newControl = make_self<SplitRoutingRulesetControl>();
            newControl->RulesetName(RulesetDialog().RulesetName());
            newControl->CanModifyRuleList(false);
            auto rules = single_threaded_observable_vector<YtFlowApp::SplitRoutingRuleModel>();
            rules.Append(make<SplitRoutingRuleModel>(L"match", SplitRoutingRuleDecision::Direct));
            rules.Append(make<SplitRoutingRuleModel>(L"unmatch", SplitRoutingRuleDecision::Direct));
            newControl->RuleList(std::move(rules));
            RulesetListView().Items().Append(*std::move(newControl));
            SelectedRulesetNameText().Text(RulesetDialog().RulesetName());
        }
    }

}
