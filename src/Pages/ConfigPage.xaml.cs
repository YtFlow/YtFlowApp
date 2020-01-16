using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using YtFlow.Tunnel.Config;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class ConfigPage : Page
    {
        private IAdapterConfig config;
        public ConfigPage ()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            config = e.Parameter as IAdapterConfig;
            Bindings.Update();
        }

        bool isSaving = false;
        private async void SaveButton_Click (object sender, RoutedEventArgs e)
        {
            if (isSaving)
            {
                return;
            }
            isSaving = true;
            try
            {
                if (string.IsNullOrEmpty(config.Path))
                {
                    var dir = await Utils.GetAdapterConfigDirectory();
                    var file = await dir.CreateFileAsync(Guid.NewGuid().ToString() + ".json", CreationCollisionOption.GenerateUniqueName);
                    config.Path = file.Path;
                }
                CachedFileManager.DeferUpdates(await StorageFile.GetFileFromPathAsync(config.Path));
                config.SaveToFile(config.Path);
                AdapterConfig.SetDefaultConfigFilePath(config.Path);
                await CachedFileManager.CompleteUpdatesAsync(await StorageFile.GetFileFromPathAsync(config.Path));
                Frame.GoBack();
            }
            catch (Exception ex)
            {
                await Utils.NotifyUser("Cannot save config: " + ex.Message);
            }
            finally
            {
                isSaving = false;
            }
        }

        private void CancelButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.GoBack();
        }

        private async void MethodComboBox_SelectionChanged (object sender, SelectionChangedEventArgs e)
        {
            if (e.AddedItems.Contains("rc4-md5"))
            {
                var dialog = new MessageDialog("The chosen cipher has inherent weaknesses. DO NOT USE.", "Warning");
                await dialog.ShowAsync();
            }
        }

        private void PasswordBox_GotFocus (object sender, RoutedEventArgs e)
        {
            ((PasswordBox)sender).PasswordRevealMode = PasswordRevealMode.Visible;
        }

        private void PasswordBox_LostFocus (object sender, RoutedEventArgs e)
        {
            ((PasswordBox)sender).PasswordRevealMode = PasswordRevealMode.Hidden;
        }
    }
}
