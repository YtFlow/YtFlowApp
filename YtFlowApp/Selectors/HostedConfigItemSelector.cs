using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Selectors
{
    class HostedConfigItemSelector : DataTemplateSelector
    {
        public DataTemplate AdapterTemplate { get; set; }
        protected override DataTemplate SelectTemplateCore (object item)
        {
            switch (item)
            {
                case IAdapterConfig _:
                    return AdapterTemplate;
            }
            return base.SelectTemplateCore(item);
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
