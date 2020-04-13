using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Selectors
{
    public class AdapterConfigTypeSelector : DataTemplateSelector
    {
        public DataTemplate ShadowsocksTemplate { get; set; }
        public DataTemplate HttpTemplate { get; set; }
        public DataTemplate TrojanTemplate { get; set; }
        protected override DataTemplate SelectTemplateCore (object item)
        {
            switch (item)
            {
                case ShadowsocksConfig _:
                    return ShadowsocksTemplate;
                case HttpConfig _:
                    return HttpTemplate;
                case TrojanConfig _:
                    return TrojanTemplate;
            }
            return base.SelectTemplateCore(item);
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
