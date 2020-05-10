using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.Foundation.Metadata;
using Windows.Networking;
using Windows.Networking.Vpn;
using Windows.Storage;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using YtFlow.App.Models;
using YtFlow.Tunnel;
using YtFlow.Tunnel.Config;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private const string PROFILE_NAME = "YtFlow Auto";
        private static readonly bool newWindowsBuild = ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7);
        public static readonly DependencyProperty TunnelConnectionStatusProperty = DependencyProperty.Register(nameof(TunnelConnectionStatus), typeof(TunnelConnectionStatus), typeof(MainPage), null);

        bool pageDisplaying;

        public TunnelConnectionStatus TunnelConnectionStatus
        {
            get => (TunnelConnectionStatus)GetValue(TunnelConnectionStatusProperty);
            set => SetValue(TunnelConnectionStatusProperty, value);
        }

        public MainPage ()
        {
            TunnelConnectionStatus = TunnelConnectionStatus.Disconnecting;

            this.InitializeComponent();
        }

        #region Tunnel Status Management
        private async Task<(VpnManagementAgent Agent, IVpnProfile Profile)> GetInstalledVpnProfile ()
        {
            var agent = new VpnManagementAgent();
            var profiles = await agent.GetProfilesAsync();
            var lowerProfileName = PROFILE_NAME.ToLower();
            var profile = profiles.FirstOrDefault(p => p.ProfileName.ToLower() == lowerProfileName);
            return (agent, profile);
        }

        private async Task CheckTunnelConnectionStatus ()
        {
            var (_, profile) = await GetInstalledVpnProfile();
            switchToggleBtn.Checked -= OnButton_Click;
            switchToggleBtn.Unchecked -= OffButton_Click;
            if (profile is VpnPlugInProfile pluginProfile)
            {
                try
                {
                    TunnelConnectionStatus = (TunnelConnectionStatus)Enum.Parse(typeof(TunnelConnectionStatus), pluginProfile.ConnectionStatus.ToString());
                }
                catch (Exception)
                {
                    // Hack for Windows Phone
                    TunnelConnectionStatus = TunnelConnectionStatus.Disconnected;
                }
            }
            else
            {
                TunnelConnectionStatus = TunnelConnectionStatus.Disconnected;
            }
            switchToggleBtn.Checked += OnButton_Click;
            switchToggleBtn.Unchecked += OffButton_Click;
        }

        private async Task<VpnManagementErrorStatus> CreateAndConnectVpn ()
        {
            var configFilePath = AdapterConfig.GetDefaultConfigFilePath();
            if (configFilePath == null)
            {
                await Utils.UiUtils.NotifyUser("Please set up a server configuration first.");
                Frame.Navigate(typeof(ConfigListPage));
                return VpnManagementErrorStatus.Other;
            }
            try
            {
                AdapterConfig.GetConfigFromFilePath(AdapterConfig.GetDefaultConfigFilePath());
            }
            catch (Exception ex)
            {
                await Utils.UiUtils.NotifyUser("Cannot load the server configuration. Reason: " + ex.ToString());
                return VpnManagementErrorStatus.Other;
            }
            var (agent, profile) = await GetInstalledVpnProfile();
            if (profile == null)
            {
                if (newWindowsBuild)
                {
                    // Create a new profile automatically
                    var newProfile = new VpnPlugInProfile()
                    {
                        AlwaysOn = false,
                        ProfileName = PROFILE_NAME,
                        RequireVpnClientAppUI = false,
                        VpnPluginPackageFamilyName = Package.Current.Id.FamilyName,
                        RememberCredentials = false
                    };
                    newProfile.ServerUris.Add(new Uri("https://github.com/YtFlow"));
                    var addResult = await agent.AddProfileFromObjectAsync(newProfile);
                    if (addResult != VpnManagementErrorStatus.Ok)
                    {
                        await Utils.UiUtils.NotifyUser("Failed to add new profile: " + addResult.ToString());
                        return VpnManagementErrorStatus.Other;
                    }
                    // profile = (await agent.GetProfilesAsync()).First(p => p.ProfileName == PROFILE_NAME);
                    profile = newProfile;
                }
                else
                {
                    // On 15063, Creaing VPN profile programmatically exhibits weird behavior.
                    // Ask users to create manually
                    var (_, msgRes) = await Utils.UiUtils.NotifyUser("VPN profile “YtFlow Auto” could not be found.\n" +
                        "Please go to Settings - Network & wireless - VPN and create a new profile with the following configuration:\n\n" +
                        "VPN Provider: " + Package.Current.DisplayName +
                        "\nConnection name: " + PROFILE_NAME +
                        "\nServer name or address: any", "First-time setup", "Go to Settings");
                    if (msgRes == ContentDialogResult.Primary)
                    {
                        await Launcher.LaunchUriAsync(new Uri("ms-settings:network-vpn"));
                    }
                    return VpnManagementErrorStatus.Other;
                }
            }
            var res = await agent.ConnectProfileAsync(profile);
            if (res != VpnManagementErrorStatus.Ok)
            {
                await Utils.UiUtils.NotifyUser("Failed to connect VPN profile: " + res.ToString());
            }
            return res;
        }

        private async Task DisconnectAndRemoveVpn ()
        {
            var (agent, profile) = await GetInstalledVpnProfile();
            if (profile == null)
            {
                return;
            }
            var res = await agent.DisconnectProfileAsync(profile);
            if (res != VpnManagementErrorStatus.Ok)
            {
                await Utils.UiUtils.NotifyUser("Cannot disconnect from VPN profile: " + res.ToString());
            }
            /*
            if (!newWindowsBuild)
            {
                return;
            }
            res = await agent.DeleteProfileAsync(profile);
            if (res != VpnManagementErrorStatus.Ok)
            {
                await Utils.NotifyUser("Cannot remove VPN profile: " + res.ToString());
            }
            */
            await DebugLogger.ResetLoggers();
        }

        private async void StartPollingTunnelConnectionStatus ()
        {
            var localSettings = ApplicationData.Current.LocalSettings.Values;
            while (pageDisplaying)
            {
                await CheckTunnelConnectionStatus();
                if (localSettings.TryGetValue("lastError", out var lastErr) && lastErr is string lastErrStr)
                {
                    await Utils.UiUtils.NotifyUser("An exception occurred during last session. Detail:\r\n\r\n" + lastErrStr, "Unexpected error");
                    localSettings.Remove("lastError");
                }
                await Task.Delay(1000);
            }
        }
        #endregion

        #region Event Handlers
        private void Page_Loaded (object sender, RoutedEventArgs e)
        {
            Frame.Navigated += Frame_Navigated;
            SystemNavigationManager.GetForCurrentView().BackRequested += MainPage_BackRequested;
            debugBtn.IsChecked = DebugLogger.IsDebugAddrSet();

            if (DebugLogger.IsDebugAddrSet())
            {
                debugHostTxt.Text = DebugLogger.RealGetDebugSocketHost() as string;
                debugPortTxt.Text = DebugLogger.RealGetDebugSocketPort() as string;
                debugBtn.IsChecked = true;
            }
            debugBtn.Checked += debugBtn_Checked;
        }

        protected override void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            pageDisplaying = true;
            StartPollingTunnelConnectionStatus();
        }

        protected override void OnNavigatedFrom (NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);

            pageDisplaying = false;
        }

        private void MainPage_BackRequested (object sender, BackRequestedEventArgs e)
        {
            if (e.Handled)
            {
                return;
            }
            if (Frame.CanGoBack)
            {
                Frame.GoBack();
                e.Handled = true;
            }
        }

        private void Frame_Navigated (object sender, NavigationEventArgs e)
        {
            var navManager = SystemNavigationManager.GetForCurrentView();
            if (Frame.CanGoBack)
            {
                navManager.AppViewBackButtonVisibility = AppViewBackButtonVisibility.Visible;
            }
            else
            {
                navManager.AppViewBackButtonVisibility = AppViewBackButtonVisibility.Disabled;
            }
        }

        private async void OnButton_Click (object sender, RoutedEventArgs e)
        {
            if (TunnelConnectionStatus != TunnelConnectionStatus.Disconnected)
            {
                return;
            }
            TunnelConnectionStatus = TunnelConnectionStatus.Connecting;
            var res = await CreateAndConnectVpn();
            if (res == VpnManagementErrorStatus.Ok)
            {
                TunnelConnectionStatus = TunnelConnectionStatus.Connected;
            }
            else
            {
                TunnelConnectionStatus = TunnelConnectionStatus.Disconnected;
            }
        }

        private async void OffButton_Click (object sender, RoutedEventArgs e)
        {
            if (TunnelConnectionStatus != TunnelConnectionStatus.Connected)
            {
                return;
            }
            TunnelConnectionStatus = TunnelConnectionStatus.Disconnecting;
            await DisconnectAndRemoveVpn();
            TunnelConnectionStatus = TunnelConnectionStatus.Disconnected;
            debugSocketRestartTxt.Visibility = Visibility.Collapsed;
        }

        private void ListView_ItemClick (object sender, ItemClickEventArgs e)
        {
            var el = (FrameworkElement)e.ClickedItem;
            switch (el.DataContext as string)
            {
                case "configList":
                    Frame.Navigate(typeof(ConfigListPage));
                    break;
                case "about":
                    Frame.Navigate(typeof(AboutPage));
                    break;
            }
        }

        private async void debugBtn_Checked (object sender, RoutedEventArgs e)
        {
            var host = debugHostTxt.Text;
            var port = debugPortTxt.Text;
            do
            {
                var isValidHost = true;
                try
                {
                    new HostName(host);
                }
                catch (Exception)
                {
                    isValidHost = false;
                }
                if (!isValidHost || !IPAddress.TryParse(host, out var _))
                {
                    await Utils.UiUtils.NotifyUser("Debug host must be a valid IP address.");
                    break;
                }
                if (!ushort.TryParse(port, out var portNum) || portNum == 0)
                {
                    await Utils.UiUtils.NotifyUser("Debug port must be a valid port number.");
                    break;
                }
                await DebugLogger.SetDebugSocketAddr(host, port);
                await DebugLogger.ResetLoggers();
                await DebugLogger.InitDebugSocket();
                debugSocketRestartTxt.Visibility = Visibility.Visible;
                return;
            } while (false);
            debugBtn.Unchecked -= debugBtn_Unchecked;
            debugBtn.IsChecked = false;
            debugBtn.Unchecked += debugBtn_Unchecked;
        }

        private async void debugBtn_Unchecked (object sender, RoutedEventArgs e)
        {
            debugSocketRestartTxt.Visibility = Visibility.Visible;
            await DebugLogger.SetDebugSocketAddr(null, null);
        }
        #endregion
    }
}
