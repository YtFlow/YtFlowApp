using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace YtFlow.App.Utils
{
    internal static class UiUtils
    {
        private static SemaphoreSlim msgboxLock = new SemaphoreSlim(1, 1);

        public static async Task<(bool SecondaryAsClose, ContentDialogResult Result)> NotifyUser(
            string content,
            string title = null,
            string primaryCommandText = null,
            string secondaryCommandText = null)
        {
            await msgboxLock.WaitAsync();
            try
            {
                return await RealNotifyUser(content, title, primaryCommandText, secondaryCommandText);
            }
            finally
            {
                msgboxLock.Release();
            }
        }

        private static async Task<(bool SecondaryAsClose, ContentDialogResult Result)> RealNotifyUser(
            string content,
            string title = null,
            string primaryCommandText = null,
            string secondaryCommandText = null)
        {
            var dialog = new ContentDialog();
            bool secondaryAsClose = false;
            dialog.Content = content;
            if (title != null)
            {
                dialog.Title = title;
            }
            if (primaryCommandText != null)
            {
                dialog.PrimaryButtonText = primaryCommandText;
            }
            if (secondaryCommandText != null)
            {
                dialog.SecondaryButtonText = secondaryCommandText;
            }

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.StandardUICommand"))
            {
                dialog.CloseButtonCommand = new StandardUICommand(StandardUICommandKind.Close);
            }
            else if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.Controls.ContentDialog", "CloseButtonText"))
            {
                dialog.CloseButtonText = "Close";
            }
            else if (secondaryCommandText == null)
            {
                secondaryAsClose = true;
                dialog.SecondaryButtonText = "Close";
            }
            return (secondaryAsClose, await dialog.ShowAsync());
        }
    }
}