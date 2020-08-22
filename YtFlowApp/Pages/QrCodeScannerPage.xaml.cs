using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.ApplicationModel.DataTransfer;
using Windows.Devices.Enumeration;
using Windows.Foundation;
using Windows.Graphics.Display;
using Windows.Graphics.Imaging;
using Windows.Media.Capture;
using Windows.Media.Devices;
using Windows.Media.MediaProperties;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using YtFlow.App.Models;
using YtFlow.App.Utils;
using ZXing;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class QrCodeScannerPage : Page
    {
        private static readonly TimeSpan FADEIN_DURATION = TimeSpan.FromMilliseconds(200);
        private MediaCapture mediaCapture = null;
        private readonly List<string> fileTypeFilter = new List<string>()
        {
            "jpg", "jpeg",
            "png",
            "bmp"
        };
        private readonly DispatcherTimer timer = new DispatcherTimer
        {
            Interval = TimeSpan.FromSeconds(2)
        };
        private readonly BarcodeReader barcodeReader = new BarcodeReader
        {
            AutoRotate = true,
            Options = new ZXing.Common.DecodingOptions
            {
                TryHarder = true,
                // https://github.com/micjahn/ZXing.Net/issues/242
                PossibleFormats = new List<BarcodeFormat>() { BarcodeFormat.QR_CODE }
            },
        };
        private Task ScanImportTask = Task.CompletedTask;
        private bool clipboardChanged = false;
        private DisplayOrientations originalPerferredOrientation;
        private Task<bool> captureInitTask = Task.FromResult(false);

        public QrCodeScannerPage ()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo (NavigationEventArgs e)
        {
            // Set device orientation to stick with the native one
            originalPerferredOrientation = DisplayInformation.AutoRotationPreferences;
            DisplayInformation.AutoRotationPreferences = DisplayInformation.GetForCurrentView().NativeOrientation;
            captureInitTask = InitVideoCapture();
            if (await captureInitTask)
            {
                InitVideoTimer();
            }
            Clipboard.ContentChanged += Clipboard_ContentChanged;
            Window.Current.Activated += CurrentWindow_Activated;
            CoreWindow.GetForCurrentThread().VisibilityChanged += CoreWindow_VisibilityChanged;

            // Trigger clipboard check once
            clipboardChanged = true;
            FromClipboard();
        }

        private async void CoreWindow_VisibilityChanged (CoreWindow sender, VisibilityChangedEventArgs args)
        {
            if (args.Visible)
            {
                captureInitTask = InitVideoCapture();
            }
            else
            {
                if (await captureInitTask)
                {
                    await mediaCapture.StopPreviewAsync();
                    mediaCapture.Dispose();
                    mediaCapture = null;
                }
            }
        }

        protected override async void OnNavigatedFrom (NavigationEventArgs e)
        {
            // Unlock device orientation
            DisplayInformation.AutoRotationPreferences = originalPerferredOrientation;
            timer.Stop();
            timer.Tick -= Timer_Tick;
            ScanImportTask = Task.CompletedTask;
            try
            {
                if (await captureInitTask)
                {
                    await mediaCapture.StopPreviewAsync();
                }
            }
            catch (Exception) { }
            mediaCapture?.Dispose();
            mediaCapture = null;
            Clipboard.ContentChanged -= Clipboard_ContentChanged;
            Window.Current.Activated -= CurrentWindow_Activated;
            CoreWindow.GetForCurrentThread().VisibilityChanged += CoreWindow_VisibilityChanged;
        }

        private void CurrentWindow_Activated (object sender, WindowActivatedEventArgs e)
        {
            if (clipboardChanged)
            {
                FromClipboard();
            }
        }

        private void Clipboard_ContentChanged (object sender, object e)
        {
            clipboardChanged = true;
            ShowWarningText("Clipboard changes detected");
        }

        private async void FromClipboard ()
        {
            await ScanImportTask;
            // If clipboardChanged becomes false, importing has done.
            if (!clipboardChanged)
            {
                // No need to scan from clipboard.
                return;
            }
            clipboardChanged = false;
            ScanImportTask = FromSharedData(Clipboard.GetContent(), true);
        }

        private async Task FromSharedData (DataPackageView content, bool prompt)
        {
            try
            {
                await FromSharedDataImpl(content, prompt);
            }
            catch (Exception ex)
            {
                await UiUtils.NotifyUser("Error reading shared content: " + ex.ToString());
            }
        }
        private async Task FromSharedDataImpl (DataPackageView content, bool prompt)
        {
            if (content.Contains(StandardDataFormats.Bitmap))
            {
                var streamRef = await content.GetBitmapAsync();
                using (var stream = await streamRef.OpenReadAsync())
                {
                    var contentType = stream.ContentType;
                    var bitmap = await ReadBitmap(stream, "." + contentType.Substring(contentType.IndexOf('/') + 1));
                    await ScanBitmap(bitmap, prompt ? "Import from copied QR Code image?" : null);
                }
            }
            else if (content.Contains(StandardDataFormats.StorageItems))
            {
                var items = await content.GetStorageItemsAsync();
                foreach (var file in items
                    .Select(f => f as StorageFile)
                    .Where(f => f != null && fileTypeFilter.Any(t => f.Name.EndsWith(t))))
                {
                    using (var stream = await file.OpenAsync(FileAccessMode.Read))
                    {
                        var writeableBmp = await ReadBitmap(stream, file.FileType);
                        await ScanBitmap(writeableBmp, prompt ? $"Import from copied file {file.Name} containing a QR Code?" : null);
                    }
                }
            }
        }

        private void InitVideoTimer ()
        {
            timer.Tick += Timer_Tick;
            timer.Start();
        }

        private async Task Timer_TickImpl (object sender, object e)
        {
            using (var stream = new InMemoryRandomAccessStream())
            {
                await mediaCapture.CapturePhotoToStreamAsync(ImageEncodingProperties.CreateJpeg(), stream);
                var writeableBmp = await ReadBitmap(stream, ".jpg");
                await ScanBitmap(writeableBmp);
            }
        }

        private void Timer_Tick (object sender, object e)
        {
            if (ScanImportTask.IsCompleted)
            {
                ScanImportTask = Timer_TickImpl(sender, e);
            }
        }

        private static Guid DecoderIDFromFileExtension (string strExtension)
        {
            Guid encoderId;
            switch (strExtension.ToLower())
            {
                case ".jpg":
                case ".jpeg":
                    encoderId = BitmapDecoder.JpegDecoderId;
                    break;

                case ".bmp":
                    encoderId = BitmapDecoder.BmpDecoderId;
                    break;

                case ".png":
                default:
                    encoderId = BitmapDecoder.PngDecoderId;
                    break;
            }
            return encoderId;
        }

        public static Size MaxSizeSupported = new Size(4000, 3000);

        /// <summary>
        /// 读取照片流 转为WriteableBitmap给二维码解码器
        /// </summary>
        /// <param name="fileStream"></param>
        /// <param name="type"></param>
        /// <returns></returns>
        public static async Task<WriteableBitmap> ReadBitmap (IRandomAccessStream fileStream, string type)
        {
            var decoderId = DecoderIDFromFileExtension(type);
            var decoder = await BitmapDecoder.CreateAsync(decoderId, fileStream);
            var tf = new BitmapTransform();

            var width = decoder.OrientedPixelWidth;
            var height = decoder.OrientedPixelHeight;
            if (decoder.OrientedPixelWidth > MaxSizeSupported.Width || decoder.OrientedPixelHeight > MaxSizeSupported.Height)
            {
                var dScale = Math.Min(MaxSizeSupported.Width / decoder.OrientedPixelWidth, MaxSizeSupported.Height / decoder.OrientedPixelHeight);
                width = (uint)(decoder.OrientedPixelWidth * dScale);
                height = (uint)(decoder.OrientedPixelHeight * dScale);
                tf.ScaledWidth = (uint)(decoder.PixelWidth * dScale);
                tf.ScaledHeight = (uint)(decoder.PixelHeight * dScale);
            }
            var bitmap = new WriteableBitmap((int)width, (int)height);
            var dataprovider = await decoder.GetPixelDataAsync(BitmapPixelFormat.Bgra8, BitmapAlphaMode.Straight, tf,
                ExifOrientationMode.RespectExifOrientation, ColorManagementMode.DoNotColorManage);
            var pixels = dataprovider.DetachPixelData();
            var pixelStream2 = bitmap.PixelBuffer.AsStream();
            pixelStream2.Write(pixels, 0, pixels.Length);

            return bitmap;
        }

        /// <summary>
        /// 解析二维码图片
        /// </summary>
        /// <param name="writeableBmp">图片</param>
        /// <param name="promptBeforeImport">导入前向用户确认的消息，null 表示不确认直接导入。</param>
        /// <returns></returns>
        private async Task ScanBitmap (WriteableBitmap writeableBmp, string promptMessage = null)
        {
            if (!(barcodeReader.Decode(writeableBmp) is Result qrResult))
            {
                return;
            }
            var text = qrResult.Text;
            if (promptMessage is string prompt)
            {
                var (_secondaryAsClose, result) = await UiUtils.NotifyUser(prompt, "QR Code detected", "Yes");
                if (result != ContentDialogResult.Primary)
                {
                    return;
                }
            }
            loadProgressBar.Visibility = Visibility.Visible;
            loadProgressBar.IsIndeterminate = true;
            LinkImportResult importResult;
            try
            {
                importResult = await ConfigUtils.ImportLinksAsync(text, p =>
                {
                    ShowWarningText("Saving...");
                    loadProgressBar.IsIndeterminate = false;
                    loadProgressBar.Value = p;
                });
            }
            finally
            {
                loadProgressBar.Visibility = Visibility.Collapsed;
            }
            if (importResult.SavedCount > 0 || importResult.FailedCount > 0)
            {
                // Avoid reading clipboard after importing
                clipboardChanged = false;
                Frame.GoBack();
                var notifyTask = UiUtils.NotifyUser(importResult.GenerateMessage());
                // Complete updates in background
                _ = importResult.Files.BatchCompleteUpdates();
                await notifyTask;
            }
            else if (importResult.UnrecognizedLines.Count > 0)
            {
                if (text.Length >= 30)
                {
                    ToolTipService.SetToolTip(warningText, text);
                    ShowWarningText($"No recognizable content. ({text.Substring(0, 30)}...)");
                }
                else
                {
                    ToolTipService.SetToolTip(warningText, null);
                    ShowWarningText($"No recognizable content. ({text})");
                }
            }
            else
            {
                ToolTipService.SetToolTip(warningText, null);
                ShowWarningText("No Content");
            }
        }

        private void ShowWarningText (string text)
        {
            if (warningText.Text == text)
            {
                if (warningTextStoryboard.GetCurrentTime() > FADEIN_DURATION)
                {
                    warningTextStoryboard.Seek(FADEIN_DURATION);
                }
            }
            else
            {
                warningText.Text = text;
                warningTextStoryboard.Begin();
            }
        }

        private async Task<bool> InitVideoCapture ()
        {
            mediaCapture = new MediaCapture();
            var cameraDevice = await FindCameraDeviceByPanelAsync(Windows.Devices.Enumeration.Panel.Back);
            if (cameraDevice == null)
            {
                await UiUtils.NotifyUser("No camera device found!");
                return false;
            }

            var settings = new MediaCaptureInitializationSettings
            {
                StreamingCaptureMode = StreamingCaptureMode.Video,
                MediaCategory = MediaCategory.Other,
                AudioProcessing = Windows.Media.AudioProcessing.Default,
                PhotoCaptureSource = PhotoCaptureSource.VideoPreview,
                VideoDeviceId = cameraDevice.Id
            };
            try
            {
                await mediaCapture.InitializeAsync(settings);
            }
            catch (UnauthorizedAccessException)
            {
                await UiUtils.NotifyUser("Please turn on the camera permission of the app to ensure scan QR code normaly.");
                return false;
            }

            var focusControl = mediaCapture.VideoDeviceController.FocusControl;
            if (focusControl.Supported)
            {
                var focusSettings = new FocusSettings()
                {
                    Mode = focusControl.SupportedFocusModes.FirstOrDefault(f => f == FocusMode.Continuous),
                    DisableDriverFallback = true,
                    AutoFocusRange = focusControl.SupportedFocusRanges.FirstOrDefault(f => f == AutoFocusRange.FullRange),
                    Distance = focusControl.SupportedFocusDistances.FirstOrDefault(f => f == ManualFocusDistance.Nearest)
                };
                focusControl.Configure(focusSettings);
            }
            VideoCapture.Source = mediaCapture;
            VideoCapture.FlowDirection = FlowDirection.LeftToRight;
            // Fix preview mirroring for front webcam
            if (cameraDevice.EnclosureLocation?.Panel == Windows.Devices.Enumeration.Panel.Front)
            {
                VideoCapture.FlowDirection = FlowDirection.RightToLeft;
            }
            await mediaCapture.StartPreviewAsync();
            // Fix preview orientation for portrait-first devices (such as Windows Phone)
            if (DisplayInformation.GetForCurrentView().NativeOrientation == DisplayOrientations.Portrait)
            {
                var props = mediaCapture.VideoDeviceController.GetMediaStreamProperties(MediaStreamType.VideoPreview);
                Guid RotationKey = new Guid("C380465D-2271-428C-9B83-ECEA3B4A85C1");
                props.Properties.Add(RotationKey, 90);
                await mediaCapture.SetEncodingPropertiesAsync(MediaStreamType.VideoPreview, props, null);
            }
            if (mediaCapture.VideoDeviceController.FlashControl.Supported)
            {
                mediaCapture.VideoDeviceController.FlashControl.Enabled = false;
            }

            if (focusControl.Supported)
            {
                await focusControl.FocusAsync();
            }
            return true;
        }

        private static async Task<DeviceInformation> FindCameraDeviceByPanelAsync (Windows.Devices.Enumeration.Panel desiredPanel)
        {
            var allVideoDevices = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
            var desiredDevice = allVideoDevices.FirstOrDefault(x => x.EnclosureLocation != null && x.EnclosureLocation.Panel == desiredPanel);
            return desiredDevice ?? allVideoDevices.FirstOrDefault();
        }

        private async void FromPictureButton_Click (object sender, RoutedEventArgs e)
        {
            var picker = new Windows.Storage.Pickers.FileOpenPicker
            {
                ViewMode = Windows.Storage.Pickers.PickerViewMode.Thumbnail,
                SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.PicturesLibrary,
                FileTypeFilter = { ".jpg", ".jpeg", ".png", ".bmp" }
            };
            var file = await picker.PickSingleFileAsync();
            if (file != null)
            {
                using (var stream = await file.OpenAsync(FileAccessMode.Read))
                {
                    var writeableBmp = await ReadBitmap(stream, file.FileType);
                    await ScanBitmap(writeableBmp);
                }
            }
        }

        private void warningTextStoryboard_Completed (object sender, object e)
        {
            warningText.Text = string.Empty;
        }

        private void ContentPanel_DragOver (object sender, DragEventArgs e)
        {
            e.AcceptedOperation = DataPackageOperation.Copy;
            if (e.DataView.Contains(StandardDataFormats.Bitmap)
                || e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                ShowWarningText("Drop to import");
            }
        }

        private void ContentPanel_Drop (object sender, DragEventArgs e)
        {
            ScanImportTask = FromSharedData(e.DataView, false);
        }
    }
}