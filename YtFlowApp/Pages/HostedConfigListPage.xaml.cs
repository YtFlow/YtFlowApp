using Microsoft.Toolkit.Uwp.UI.Extensions;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;
using YtFlow.App.Models;
using YtFlow.App.Utils;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class HostedConfigListPage : Page
    {
        private bool configLoading = false;
        private readonly ObservableCollection<HostedConfigListItem> hostedConfigListItems =
            new ObservableCollection<HostedConfigListItem>();
        internal static HostedConfigListItem itemForBackNavigation;
        private readonly DispatcherTimer humanizerUpdateTimer = new DispatcherTimer()
        {
            Interval = TimeSpan.FromMinutes(1d)
        };

        public HostedConfigListPage ()
        {
            this.InitializeComponent();

            _ = LoadConfigs();
        }

        protected override void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            humanizerUpdateTimer.Tick += HumanizerUpdateTimer_Tick;
            humanizerUpdateTimer.Start();

            var animation = ConnectedAnimationService.GetForCurrentView().GetAnimation("hostedConfigName");
            if (animation != null && itemForBackNavigation != null)
            {
                _ = hostedConfigList.TryStartConnectedAnimationAsync(animation, itemForBackNavigation, "nameText");
            }
        }

        protected override void OnNavigatedFrom (NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);

            humanizerUpdateTimer.Tick -= HumanizerUpdateTimer_Tick;
            humanizerUpdateTimer.Stop();
        }

        private void HumanizerUpdateTimer_Tick (object sender, object e)
        {
            foreach (var item in hostedConfigListItems)
            {
                item.TriggerTimeUpdate();
            }
        }

        private async Task LoadConfigs ()
        {
            if (configLoading)
            {
                return;
            }
            configLoading = true;
            try
            {
                var dir = await HostedUtils.HostedConfigFolderTask;
                var files = await dir.GetFilesAsync();
                var configs = await Task.WhenAll(files.Select(f => HostedUtils.GetHostedConfigAsync(f)));
                hostedConfigListItems.Clear();
                foreach (var config in configs)
                {
                    hostedConfigListItems.Add(new HostedConfigListItem(config));
                }
            }
            catch (Exception ex)
            {
                await UiUtils.NotifyUser("Error loading hosted config list: " + ex.ToString());
                Frame.GoBack();
            }
            finally
            {
                configLoading = false;
            }
        }

        private void AddButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(NewHostedConfigPage), hostedConfigListItems);
        }

        private void hostedConfigList_ItemClick (object sender, ItemClickEventArgs e)
        {
            var item = (HostedConfigListItem)e.ClickedItem;
            hostedConfigList.PrepareConnectedAnimation("hostedConfigName", item, "nameText");
            itemForBackNavigation = item;
            Frame.Navigate(typeof(HostedConfigPage), item);
        }

        private void RenameFlyoutItem_Click (object sender, RoutedEventArgs e)
        {
            var container = (ListViewItem)hostedConfigList.ContainerFromItem(((FrameworkElement)sender).DataContext);
            var nameText = container.FindDescendantByName("nameText");
            Flyout.ShowAttachedFlyout(nameText);
        }

        private void RenameTextBox_KeyDown (object sender, KeyRoutedEventArgs args)
        {
            if (args.Handled)
            {
                return;
            }
            if (args.Key != Windows.System.VirtualKey.Enter)
            {
                return;
            }
            args.Handled = true;
            var textBox = (TextBox)sender;
            var item = (HostedConfigListItem)textBox.DataContext;
            _ = item.Rename(textBox.Text);
            var nameText = hostedConfigList.ContainerFromItem(item).FindDescendantByName("nameText");
            Flyout.GetAttachedFlyout(nameText).Hide();
        }

        private async void RemoveFlyoutItem_Click (object sender, RoutedEventArgs e)
        {
            var item = (HostedConfigListItem)((FrameworkElement)sender).DataContext;
            var (_, result) = await UiUtils.NotifyUser("Remove this hosted configuration?", primaryCommandText: "Yes");
            if (result != ContentDialogResult.Primary)
            {
                return;
            }
            try
            {
                await item.DeleteAsync();
                hostedConfigListItems.Remove(item);
            }
            catch (Exception ex)
            {
                await UiUtils.NotifyUser("Error while removing hosted configuration: " + ex.ToString());
            }
        }
    }
}
