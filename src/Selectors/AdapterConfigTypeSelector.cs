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
                case ShadowsocksConfig ss:
                    return ShadowsocksTemplate;
                case HttpConfig ht:
                    return HttpTemplate;
                case TrojanConfig tj:
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
