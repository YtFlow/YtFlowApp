#include "pch.h"
#include "NewProfilePage.h"
#if __has_include("NewProfilePage.g.cpp")
#include "NewProfilePage.g.cpp"
#endif

#include "CoreFfi.h"
#include "NewProfileRulesetControl.h"
#include "RawEditorParam.h"
#include "SplitRoutingRulesetControl.h"
#include "UI.h"

using namespace nlohmann;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Controls;

namespace winrt::YtFlowApp::implementation
{
    struct NewProfileConfigRuleset
    {
        std::string Name;
        std::vector<std::pair<std::string, SplitRoutingRuleDecision>> Rules{};
        bool IsList{};
        SplitRoutingRuleDecision FallbackRule{};
    };
    struct NewProfileConfig
    {
        hstring InboundMode;
        hstring RuleResolver;
        hstring OutboundType;
        std::string CustomRules;
        std::vector<NewProfileConfigRuleset> Rulesets{};
    };
    void CreatePresetPlugins(uint32_t profileId, NewProfileConfig config);

    NewProfilePage::NewProfilePage()
    {
        // InitializeComponent();
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
        NewProfileNameText().IsEnabled(true);
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

            SaveButton().IsEnabled(false);
            const auto lifetime{get_strong()};
            bool nameDuplicated{false};
            NewProfileConfig config{
                .InboundMode = InboundModeButtons().SelectedItem().as<RadioButton>().Tag().as<hstring>(),
                .RuleResolver = RuleResolverComboBox().SelectedItem().as<ComboBoxItem>().Tag().as<hstring>(),
                .OutboundType = m_selectedOutboundType,
                .CustomRules = to_string(CustomRuleTextBox().Text()),
                .Rulesets =
                    std::views::transform(
                        RulesetListView().Items(),
                        [](auto const &item) {
                            auto const control =
                                get_self<SplitRoutingRulesetControl>(item.as<YtFlowApp::SplitRoutingRulesetControl>());
                            return NewProfileConfigRuleset{
                                .Name = to_string(control->RulesetName()),
                                .Rules = std::views::transform(
                                             control->RuleList(),
                                             [](auto const &ruleModel) {
                                                 auto const ruleObj = get_self<SplitRoutingRuleModel>(ruleModel);
                                                 return std::pair{to_string(ruleObj->Rule()), ruleObj->Decision()};
                                             }) |
                                         std::ranges::to<std::vector>(),
                                .IsList = !control->CanModifyRuleList(),
                                .FallbackRule = control->FallbackRule().Decision(),
                            };
                        }) |
                    std::ranges::to<std::vector>(),
            };

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
            SaveButton().IsEnabled(true);
            if (nameDuplicated)
            {
                NewProfileNameText().IsEnabled(true);
                NewProfileNameText().Foreground(Media::SolidColorBrush{Windows::UI::Colors::Red()});
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
    fire_and_forget NewProfilePage::SplitRoutingModeButtons_SelectionChanged(IInspectable const &,
                                                                             SelectionChangedEventArgs const &e)
    {
        auto const lifetime = get_strong();
        auto const selectedIt = e.AddedItems().First();
        if (!selectedIt.HasCurrent())
        {
            co_return;
        }
        auto const selected = selectedIt.Current().as<FrameworkElement>();
        if (!selected)
        {
            co_return;
        }
        auto const selectedMode = selected.Tag().as<hstring>();
        static int32_t lastSelectedIndex = 0; // Workaround for e.RemovedItems().First() -> nullptr
        auto const idx = SplitRoutingModeButtons().SelectedIndex();
        if (lastSelectedIndex == idx)
        {
            co_return;
        }

        bool const splitRoutingEnabled = selectedMode != L"all";
        if (splitRoutingEnabled)
        {
            if (co_await DownloadRulesetConsentDialog().ShowAsync() != ContentDialogResult::Primary)
            {
                SplitRoutingModeButtons().SelectedIndex(lastSelectedIndex);
                co_return;
            }
        }

        RulesetListView().Items().Clear();
        if (!splitRoutingEnabled)
        {
            lastSelectedIndex = 0;
            co_return;
        }

        (void)RulesetDialog().ShowAsync();
        bool updated{};
        if (selectedMode == L"whitelist")
        {
            updated = co_await get_self<NewProfileRulesetControl>(RulesetDialog())
                          ->BatchUpdateRulesetsIfNotExistAsync({
                              L"loyalsoldier-country-only-cn-private",
                              L"loyalsoldier-surge-proxy",
                              L"loyalsoldier-surge-direct",
                              L"loyalsoldier-surge-private",
                              L"loyalsoldier-surge-reject",
                          });
            if (updated)
            {
                AddListRuleset(L"loyalsoldier-surge-private", SplitRoutingRuleDecision::Direct);
                AddListRuleset(L"loyalsoldier-surge-reject", SplitRoutingRuleDecision::Reject);
                AddListRuleset(L"loyalsoldier-surge-proxy", SplitRoutingRuleDecision::Proxy);
                AddListRuleset(L"loyalsoldier-surge-direct", SplitRoutingRuleDecision::Direct);
                AddRuleRuleset(L"loyalsoldier-country-only-cn-private", L"cn", SplitRoutingRuleDecision::Direct);
            }
        }
        else if (selectedMode == L"blacklist")
        {
            updated = co_await get_self<NewProfileRulesetControl>(RulesetDialog())
                          ->BatchUpdateRulesetsIfNotExistAsync({
                              L"loyalsoldier-surge-proxy",
                              L"loyalsoldier-surge-tld-not-cn",
                              L"loyalsoldier-surge-private",
                              L"loyalsoldier-surge-reject",
                          });
            if (updated)
            {
                AddListRuleset(L"loyalsoldier-surge-private", SplitRoutingRuleDecision::Direct);
                AddListRuleset(L"loyalsoldier-surge-reject", SplitRoutingRuleDecision::Reject);
                AddListRuleset(L"loyalsoldier-surge-proxy", SplitRoutingRuleDecision::Proxy);
                AddListRuleset(L"loyalsoldier-surge-tld-not-cn", SplitRoutingRuleDecision::Proxy,
                               SplitRoutingRuleDecision::Direct);
            }
        }
        else if (selectedMode == L"overseas")
        {
            updated = co_await get_self<NewProfileRulesetControl>(RulesetDialog())
                          ->BatchUpdateRulesetsIfNotExistAsync({
                              L"loyalsoldier-country-only-cn-private",
                              L"loyalsoldier-surge-proxy",
                              L"loyalsoldier-surge-direct",
                              L"loyalsoldier-surge-tld-not-cn",
                              L"loyalsoldier-surge-private",
                              L"loyalsoldier-surge-reject",
                          });
            if (updated)
            {
                AddListRuleset(L"loyalsoldier-surge-private", SplitRoutingRuleDecision::Direct);
                AddListRuleset(L"loyalsoldier-surge-reject", SplitRoutingRuleDecision::Reject);
                AddListRuleset(L"loyalsoldier-surge-proxy", SplitRoutingRuleDecision::Direct);
                AddListRuleset(L"loyalsoldier-surge-direct", SplitRoutingRuleDecision::Proxy);
                AddListRuleset(L"loyalsoldier-surge-tld-not-cn", SplitRoutingRuleDecision::Direct);
                AddRuleRuleset(L"loyalsoldier-country-only-cn-private", L"cn", SplitRoutingRuleDecision::Proxy,
                               SplitRoutingRuleDecision::Direct);
            }
        }
        if (updated)
        {
            RulesetDialog().Hide();
            lastSelectedIndex = idx;
        }
        else
        {
            SplitRoutingModeButtons().SelectedIndex(lastSelectedIndex);
        }
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

    void CreatePresetPlugins(uint32_t profileId, NewProfileConfig config)
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        try
        {
            auto doc = NewProfilePage::GenPresetDoc();

            if (config.InboundMode == L"full")
            {
                doc["uwp-vpn-tun"]["param"]["ipv4_route"] = {"0.0.0.0/1", "128.0.0.0/1"};
                doc["uwp-vpn-tun"]["param"]["ipv6_route"] = {"::/1", "8000::/1"};
            }

            // Determine resolver for rules
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

            // Compose ruleset chain
            std::string tcpNext = "proxy-forward.tcp", udpNext = "resolve-proxy.udp";
            unsigned int rulesetIdx{};
            auto const makeAction = [&](SplitRoutingRuleDecision decision) -> json::object_t {
                switch (decision)
                {
                case SplitRoutingRuleDecision::Proxy:
                    return {{"tcp", "proxy-forward.tcp"}, {"udp", "resolve-proxy.udp"}};
                case SplitRoutingRuleDecision::Direct:
                    return {{"tcp", "direct-forward.tcp"}, {"udp", "resolve-local.udp"}};
                case SplitRoutingRuleDecision::Reject:
                    return {{"tcp", "reject.tcp"}, {"udp", "reject.udp"}};
                case SplitRoutingRuleDecision::Next:
                    return {{"tcp", tcpNext}, {"udp", udpNext}};
                }
                throw std::invalid_argument("Invalid decision");
            };
            auto const decisionName = [](SplitRoutingRuleDecision decision) {
                switch (decision)
                {
                case SplitRoutingRuleDecision::Proxy:
                    return "proxy";
                case SplitRoutingRuleDecision::Direct:
                    return "direct";
                case SplitRoutingRuleDecision::Reject:
                    return "reject";
                case SplitRoutingRuleDecision::Next:
                    return "next";
                }
                throw std::invalid_argument("Invalid decision");
            };
            auto const availableActions = [&]() -> json::object_t {
                return {{"proxy", makeAction(SplitRoutingRuleDecision::Proxy)},
                        {"direct", makeAction(SplitRoutingRuleDecision::Direct)},
                        {"reject", makeAction(SplitRoutingRuleDecision::Reject)},
                        {"next", makeAction(SplitRoutingRuleDecision::Next)}};
            };
            for (auto &ruleset : config.Rulesets | std::views::reverse)
            {
                json rulesetDoc{{"desc", "Rule dispatcher for ruleset "s + ruleset.Name + "."},
                                {"plugin", ruleset.IsList ? "list-dispatcher" : "rule-dispatcher"},
                                {"plugin_version", 0},
                                {"param",
                                 {{"resolver", ruleResolver},
                                  {"source", ruleset.Name},
                                  {"fallback", makeAction(ruleset.FallbackRule)}}}};
                if (ruleset.IsList)
                {
                    rulesetDoc["param"]["action"] = makeAction(ruleset.Rules.at(0).second);
                }
                else
                {
                    rulesetDoc["param"]["actions"] = availableActions();
                    rulesetDoc["param"]["rules"] =
                        std::move(ruleset.Rules) | std::views::transform([&](auto &&rule) {
                            return std::make_pair(std::move(rule.first), decisionName(rule.second));
                        }) |
                        std::ranges::to<std::map>();
                }
                std::string pluginName = "ruleset-dispatcher-"s + std::to_string(++rulesetIdx);
                doc[pluginName] = std::move(rulesetDoc);
                tcpNext = pluginName + ".tcp";
                udpNext = std::move(pluginName) + ".udp";
            }
            json customRuleDoc{
                {"desc", "Rule dispatcher for custom rules"},
                {"plugin", "rule-dispatcher"},
                {"plugin_version", 0},
                {"param",
                 {{"resolver", ruleResolver},
                  {"source",
                   {{"format", "quanx-filter"},
                    {"text", std::views::split(std::string_view(config.CustomRules), "\r"sv) |
                                 std::views::transform([](auto const sr) {
                                     std::string_view const sv(sr);
                                     auto const rpos = sv.find_last_not_of('\r');
                                     return sv.substr(0, rpos == std::string_view::npos ? sv.size() : rpos + 1);
                                 }) |
                                 std::ranges::to<std::vector>()}}},
                  {"actions", availableActions()},
                  {"rules", {{"proxy", "proxy"}, {"direct", "direct"}, {"reject", "reject"}, {"next", "next"}}},
                  {"fallback", makeAction(SplitRoutingRuleDecision::Next)}}}};
            doc["custom-rule-dispatcher"] = std::move(customRuleDoc);

            // Adjust outbound
            if (config.OutboundType == L"ss")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "ss-client.tcp";
                doc["proxy-forward"]["param"]["udp_next"] = "ss-client.udp";
            }
            else if (config.OutboundType == L"http")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "http-proxy-client.tcp";
            }
            else if (config.OutboundType == L"trojan")
            {
                doc["proxy-forward"]["param"]["tcp_next"] = "trojan-client.tcp";
                doc["proxy-forward"]["param"]["udp_next"] = "trojan-client.udp";
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

    void NewProfilePage::AddListRuleset(hstring name, SplitRoutingRuleDecision const match,
                                        SplitRoutingRuleDecision const unmatch)
    {
        auto newControl = make_self<SplitRoutingRulesetControl>();
        newControl->RulesetName(std::move(name));
        newControl->CanModifyRuleList(false);
        auto rules = single_threaded_observable_vector<YtFlowApp::SplitRoutingRuleModel>();
        rules.Append(make<SplitRoutingRuleModel>(L"match", match));
        newControl->FallbackRule(make<SplitRoutingRuleModel>(L"unmatch", unmatch));
        newControl->RuleList(std::move(rules));
        newControl->RemoveRequested([weak = get_weak()](auto const &control, auto const &) {
            if (auto const self{weak.get()})
            {
                uint32_t idx{};
                if (self->RulesetListView().Items().IndexOf(control, idx))
                {
                    self->RulesetListView().Items().RemoveAt(idx);
                }
            }
        });
        RulesetListView().Items().Append(*std::move(newControl));
    }
    void NewProfilePage::AddRuleRuleset(hstring name, hstring matchRule, SplitRoutingRuleDecision const match,
                                        SplitRoutingRuleDecision const unmatch)
    {
        auto newControl = make_self<SplitRoutingRulesetControl>();
        newControl->RulesetName(std::move(name));
        newControl->CanModifyRuleList(true);
        auto rules = single_threaded_observable_vector<YtFlowApp::SplitRoutingRuleModel>();
        rules.Append(make<SplitRoutingRuleModel>(std::move(matchRule), match));
        newControl->FallbackRule(make<SplitRoutingRuleModel>(L"unmatch", unmatch));
        newControl->RuleList(std::move(rules));
        newControl->RemoveRequested([weak = get_weak()](auto const &control, auto const &) {
            if (auto const self{weak.get()})
            {
                uint32_t idx{};
                if (self->RulesetListView().Items().IndexOf(control, idx))
                {
                    self->RulesetListView().Items().RemoveAt(idx);
                }
            }
        });
        RulesetListView().Items().Append(*std::move(newControl));
    }

    fire_and_forget NewProfilePage::AddRulesetButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        co_await RulesetDialog().ShowAsync();
        if (RulesetDialog().RulesetSelected())
        {
            auto const rulesetName = RulesetDialog().RulesetName();
            auto const rulesetNameView = static_cast<std::wstring_view>(rulesetName);
            if (rulesetNameView.contains(L"country") || rulesetNameView.contains(L"geoip"))
            {
                AddRuleRuleset(std::move(rulesetName), L"cn", SplitRoutingRuleDecision::Direct);
            }
            else
            {
                AddListRuleset(std::move(rulesetName), SplitRoutingRuleDecision::Direct);
            }
        }
    }

    void NewProfilePage::CreateCustomRuleButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        CreateCustomRuleButton().Visibility(Visibility::Collapsed);
        CustomRuleTextBox().Text(
            hstring(L"# See https://ytflow.github.io/ytflow-book/plugins/rule-dispatcher.html\r\n") +
            L"# for quanx-filter-based custom rule syntax.\r\n\r\n" + L"domain, www.example.com, direct\r\n" +
            L"domain-suffix, ip.sb, proxy\r\n" + L"domain-keyword, google-analytics, reject\r\n" +
            L"ip-cidr, 114.114.114.114/32, direct, no-resolve\r\n" +
            L"ip6-cidr, 2001:4860:4860::8800/120, proxy, no-resolve\r\n");
        CustomRuleTextBox().Visibility(Visibility::Visible);
    }
}
