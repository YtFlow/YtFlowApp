using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage ()
        {
            this.InitializeComponent();
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
            }
        }

        private void Frame_Navigated (object sender, NavigationEventArgs e)
        {
            var navManager = SystemNavigationManager.GetForCurrentView();
            if (Frame.CanGoBack)
            {
                navManager.AppViewBackButtonVisibility = AppViewBackButtonVisibility.Visible;
            }
        }

        private void HyperlinkButton_Click (object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(ConfigListPage));
        }

        private void Page_Loaded (object sender, RoutedEventArgs e)
        {
            Frame.Navigated += Frame_Navigated;
            SystemNavigationManager.GetForCurrentView().BackRequested += MainPage_BackRequested;
        }
    }
}
