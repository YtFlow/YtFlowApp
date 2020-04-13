using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.App.Models;

namespace YtFlow.App.Selectors
{
    public class ConnectionStatusSelector : DataTemplateSelector
    {
        public DataTemplate DisconnectedTemplate { get; set; }
        public DataTemplate ConnectingTemplate { get; set; }
        public DataTemplate ConnectedTemplate { get; set; }
        public DataTemplate DisconnectingTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore (object item)
        {
            switch (item as TunnelConnectionStatus?)
            {
                case TunnelConnectionStatus.Disconnected:
                    return DisconnectedTemplate;
                case TunnelConnectionStatus.Connecting:
                    return ConnectingTemplate;
                case TunnelConnectionStatus.Connected:
                    return ConnectedTemplate;
                case TunnelConnectionStatus.Disconnecting:
                    return DisconnectingTemplate;
            }
            return base.SelectTemplateCore(item);
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
