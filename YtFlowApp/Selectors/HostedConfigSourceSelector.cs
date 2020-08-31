using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.Tasks.Hosted.Source;

namespace YtFlow.App.Selectors
{
    class HostedConfigSourceSelector : DataTemplateSelector
    {
        public DataTemplate UrlTemplate { get; set; }
        public DataTemplate FileTemplate { get; set; }
        protected override DataTemplate SelectTemplateCore (object item)
        {
            switch (item)
            {
                case UrlSource _:
                    return UrlTemplate;
                case FileSource _:
                    return FileTemplate;
            }
            return base.SelectTemplateCore(item);
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
