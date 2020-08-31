using System;
using System.Linq;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;
using YtFlow.App.Models;
using YtFlow.Tasks.Hosted;
using YtFlow.Tunnel.Config;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class HostedConfigPage : Page
    {
        private const string CONNECTED_ANIMATION_HOSTED_CONFIG_NAME = "hostedConfigName";
        private static double _persistedHeight = -1;
        private static string _persistedItemKey = "";
        private static string _persistedPosition = "";

        private HostedConfigListItem HostedConfigListItem { get; set; }

        public HostedConfigPage ()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            HostedConfigListItem = (HostedConfigListItem)e.Parameter;
            var nameAnimation = ConnectedAnimationService.GetForCurrentView().GetAnimation(CONNECTED_ANIMATION_HOSTED_CONFIG_NAME);
            if (nameAnimation != null)
            {
                nameAnimation.TryStart(hostedConfigNameTextBlock);
            }
            Bindings.Update();
        }

        protected override void OnNavigatingFrom (NavigatingCancelEventArgs e)
        {
            _persistedPosition = ListViewPersistenceHelper.GetRelativeScrollPosition(itemGridView, GetKey);
            base.OnNavigatingFrom(e);

            if (e.Cancel)
            {
                return;
            }
            if (e.NavigationMode == NavigationMode.Back)
            {
                _persistedPosition = null;
                var animation = ConnectedAnimationService.GetForCurrentView()
                    .PrepareToAnimate(CONNECTED_ANIMATION_HOSTED_CONFIG_NAME, hostedConfigNameTextBlock);
                if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.Animation.DirectConnectedAnimationConfiguration"))
                {
                    animation.Configuration = new DirectConnectedAnimationConfiguration();
                }
            }
        }

        private void Page_Loaded (object sender, RoutedEventArgs e)
        {
            if (!string.IsNullOrEmpty(_persistedPosition))
            {
                _ = ListViewPersistenceHelper.SetRelativeScrollPositionAsync(itemGridView, _persistedPosition, GetItem);
            }
        }

        private IAsyncOperation<object> GetItem (string key)
        {
            return Task.Run(async () =>
            {
                Snapshot snapshot;
                try
                {
                    snapshot = await HostedConfigListItem.LoadSnapshotTask.Task;
                }
                catch (Exception)
                {
                    return null;
                }
                return (object)snapshot.AdapterConfigs.FirstOrDefault(i => i.Name == key);
            }).AsAsyncOperation();
        }

        private void itemGridView_ItemClick (object sender, ItemClickEventArgs e)
        {
            if (e.ClickedItem is IAdapterConfig adapter)
            {
                Frame.Navigate(typeof(ConfigPage), adapter);
            }
        }

        private string GetKey (object item)
        {
            if (item is IAdapterConfig adapter)
            {
                _persistedHeight = ((GridViewItem)itemGridView.ContainerFromItem(item)).ActualHeight;
                _persistedItemKey = adapter.Name;
                return _persistedItemKey;
            }
            return string.Empty;
        }

        private void UpdateButton_Click (object sender, RoutedEventArgs e)
        {
            _ = HostedConfigListItem.UpdateAsync();
        }

        private void DismissErrorButton_Click (object sender, RoutedEventArgs e)
        {
            HostedConfigListItem.DismissError();
        }

        private void HostedConfigNameTextBlock_Tapped (object sender, Windows.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
            Flyout.ShowAttachedFlyout((FrameworkElement)sender);
        }

        private void RenameTextBox_KeyDown (object sender, Windows.UI.Xaml.Input.KeyRoutedEventArgs args)
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
            _ = HostedConfigListItem.Rename(textBox.Text);
            Flyout.GetAttachedFlyout(hostedConfigNameTextBlock).Hide();
        }

        private void ItemGridView_ContainerContentChanging (ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            string key;
            switch (args.Item)
            {
                case IAdapterConfig adapter:
                    key = adapter.Name;
                    break;
                default:
                    return;
            }
            if (key != _persistedItemKey)
            {
                return;
            }

            if (!args.InRecycleQueue)
            {
                args.ItemContainer.Height = _persistedHeight;
            }
            else
            {
                args.ItemContainer.ClearValue(HeightProperty);
            }
        }
    }
}
