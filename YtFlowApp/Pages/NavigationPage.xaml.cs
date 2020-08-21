using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class NavigationPage : Page
    {
        private static Dictionary<string, Type> NavigationItemContentToPageType = new Dictionary<string, Type>()
        {
            { "Home", typeof(MainPage) },
            { "About", typeof(AboutPage) }
        };

        public NavigationPage ()
        {
            this.InitializeComponent();
        }

        private void Page_Loaded (object sender, RoutedEventArgs e)
        {
            navFrame.Navigate(typeof(MainPage));
        }

        private void navView_SelectionChanged (Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewSelectionChangedEventArgs args)
        {
            if (args.IsSettingsSelected)
            {
                navFrame.Navigate(typeof(SettingPage));
            }
            else if (args.SelectedItem is Microsoft.UI.Xaml.Controls.NavigationViewItem item)
            {
                navFrame.Navigate(NavigationItemContentToPageType[item.Content.ToString()]);
            }
        }
    }
}
