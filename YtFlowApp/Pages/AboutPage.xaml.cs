using Windows.ApplicationModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class AboutPage : Page
    {
        public AboutPage ()
        {
            this.InitializeComponent();
        }

        private void Page_Loaded (object sender, RoutedEventArgs e)
        {
            var package = Package.Current;
            var version = package.Id.Version;
            versionTxt.Text = $"{version.Major}.{version.Minor}.{version.Build}.{version.Revision}";
            buildTxt.Text =
#if DEBUG
                "Debug"
#else
                "Release"
#endif
                ;
        }
    }
}
