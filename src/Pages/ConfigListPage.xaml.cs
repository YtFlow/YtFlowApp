using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using YtFlow.Tunnel.Config;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class ConfigListPage : Page
    {
        public ObservableCollection<IAdapterConfig> adapterConfigs { get; set; } = new ObservableCollection<IAdapterConfig>();

        public ConfigListPage ()
        {
            this.InitializeComponent();
        }

        private async Task LoadAdapterConfigs ()
        {
            var directory = await Utils.GetAdapterConfigDirectory();
            var files = await directory.GetFilesAsync();
            adapterConfigs.Clear();
            foreach (var config in files.Select(f => AdapterConfig.GetConfigFromFilePath(f.Path)))
            {
                adapterConfigs.Add(config);
            }
        }

        protected override async void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            await LoadAdapterConfigs();
        }

        private void AdapterListView_ItemClick (object sender, ItemClickEventArgs e)
        {
            Frame.Navigate(typeof(ConfigPage), e.ClickedItem);
        }

        private async void SetDefaultButton_Click (object sender, RoutedEventArgs e)
        {
            var btn = (MenuFlyoutItem)sender;
            var defaultConfig = (IAdapterConfig)btn.DataContext;
            AdapterConfig.SetDefaultConfigFilePath(defaultConfig.Path);
            await LoadAdapterConfigs();
        }

        private async void RemoveButton_Click (object sender, RoutedEventArgs e)
        {
            var btn = (MenuFlyoutItem)sender;
            var config = (IAdapterConfig)btn.DataContext;
            var (secondaryAsClose, result) = await Utils.NotifyUser("Remove this config?", primaryCommandText: "Yes");
            if (result != ContentDialogResult.Primary)
            {
                return;
            }
            try
            {
                File.Delete(config.Path);
                adapterConfigs.Remove(config);
                if (config.Path == AdapterConfig.GetDefaultConfigFilePath() && adapterConfigs.Count > 0)
                {
                    AdapterConfig.SetDefaultConfigFilePath(adapterConfigs[0].Path);
                }
            }
            catch (Exception ex)
            {
                await Utils.NotifyUser("Error while deleting config file: " + ex.Message);
            }
        }

        private void CreateShadowsocksButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(ConfigPage), new ShadowsocksConfig()
            {
                ServerHost = "la.maxlv.net",
                ServerPort = 1080,
                Password = "CCCCCCCC",
                Method = "aes-128-cfb",
                Name = "Connection " + (adapterConfigs.Count + 1)
            });
        }

        private void CreateHttpButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(ConfigPage), new HttpConfig()
            {
                ServerHost = "127.0.0.1",
                ServerPort = 80,
                Name = "Connection " + (adapterConfigs.Count + 1)
            });
        }

        private void CreateTrojanButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(ConfigPage), new TrojanConfig()
            {
                ServerHost = "127.0.0.1",
                ServerPort = 443,
                Name = "Connection " + (adapterConfigs.Count + 1)
            });
        }
    }
}
