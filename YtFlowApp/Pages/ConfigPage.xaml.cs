using System;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.ApplicationModel.DataTransfer;
using Windows.Graphics.Imaging;
using Windows.Storage;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using YtFlow.App.ConfigEncoding;
using YtFlow.App.Utils;
using YtFlow.Tunnel.Config;
using ZXing;
using ZXing.Common;
using ZXing.Rendering;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class ConfigPage : Page
    {
        private IAdapterConfig config;
        private string shareLink;
        public ConfigPage ()
        {
            this.InitializeComponent();
        }

        private void PrepareShare ()
        {
            try
            {
                var newLink = config.Encode();
                if (newLink == shareLink)
                {
                    return;
                }
                shareLink = newLink;
                var barWriter = new BarcodeWriter
                {
                    Format = BarcodeFormat.QR_CODE,
                    Options = new EncodingOptions() { Height = 200, Width = 200 },
                    Renderer = new WriteableBitmapRenderer()
                };
                qrImage.Source = barWriter.Write(shareLink);
                shareButton.IsEnabled = true;
            }
            catch (NotSupportedException)
            {
                shareButton.IsEnabled = false;
            }
            catch (Exception ex)
            {
                shareButton.DataContext = ex;
                shareButton.IsEnabled = true;
            }
        }

        protected override void OnNavigatedTo (NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            config = e.Parameter as IAdapterConfig;
            PrepareShare();
            Bindings.Update();
        }

        protected override void OnNavigatedFrom (NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);

            qrImage.Source = null;
        }

        bool isSaving = false;
        private void SaveButton_Click (object sender, RoutedEventArgs e)
        {
            _ = SaveAsync(false);
        }
        private async Task SaveAsync (bool setAsDefault)
        {
            if (isSaving)
            {
                return;
            }
            isSaving = true;
            try
            {
                var file = await ConfigUtils.SaveServerAsync(config);
                config.Path = file.Path;
                if (setAsDefault)
                {
                    AdapterConfig.SetDefaultConfigFilePath(config.Path);
                }
                await CachedFileManager.CompleteUpdatesAsync(file);
                Frame.GoBack();
            }
            catch (Exception ex)
            {
                await UiUtils.NotifyUser("Cannot save config: " + ex.Message);
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
            if (e.RemovedItems.Count > 0 && e.AddedItems.Contains("rc4-md5"))
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

        private async void ShareButton_Click (object sender, RoutedEventArgs e)
        {
            PrepareShare();
            if (((FrameworkElement)sender).DataContext is Exception ex)
            {
                await UiUtils.NotifyUser(ex.Message);
                return;
            }
            FlyoutBase.ShowAttachedFlyout((FrameworkElement)sender);
        }

        private async void CopyLinkItem_Click (object sender, RoutedEventArgs e)
        {
            var dataPackage = new DataPackage
            {
                RequestedOperation = DataPackageOperation.Copy
            };
            dataPackage.SetText(shareLink);
            Clipboard.SetContent(dataPackage);
            // Make copied content persistent
            Clipboard.Flush();
            await UiUtils.NotifyUser("Copied to clipboard.");
        }

        private void ShareImageButton_Click (object sender, RoutedEventArgs e)
        {
            var manager = DataTransferManager.GetForCurrentView();
            async void Manager_DataRequested (DataTransferManager managerSender, DataRequestedEventArgs args)
            {
                manager.DataRequested -= Manager_DataRequested;
                var def = args.Request.GetDeferral();
                var reqData = args.Request.Data;
                try
                {
                    var pixelBuffer = ((WriteableBitmap)qrImage.Source).PixelBuffer;
                    // A temporary file is used for compatibility
                    var tempFile = await ApplicationData.Current.TemporaryFolder.CreateFileAsync("qrcode.png", CreationCollisionOption.ReplaceExisting);
                    using (var stream = await tempFile.OpenAsync(FileAccessMode.ReadWrite))
                    {
                        var encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.PngEncoderId, stream);
                        encoder.SetPixelData(BitmapPixelFormat.Bgra8, BitmapAlphaMode.Ignore,
                            200, 200, 96, 96, pixelBuffer.ToArray());
                        await encoder.FlushAsync();
                        await stream.FlushAsync();
                    }
                    // SetBitmap avoids ShareUI being shown on Windows Phone
                    // Moreover, it makes no difference on Windows Desktop
                    // Use SetStorageItems instead
                    reqData.SetStorageItems(new IStorageItem[] { tempFile }, true);
                    reqData.Properties.Title = "QR Code for Server Config";
                    reqData.Properties.Description = config.Name;
                }
                finally
                {
                    def.Complete();
                }
            }
            manager.DataRequested += Manager_DataRequested;
            DataTransferManager.ShowShareUI();
        }

        private void SaveAsDefaultButton_Click (object sender, RoutedEventArgs e)
        {
            _ = SaveAsync(true);
        }
    }
}
