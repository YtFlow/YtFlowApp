using System;
using Windows.UI.Xaml.Data;
using YtFlow.App.Models;

namespace YtFlow.App.Converters
{
    class ConnectionStatusToIsCheckedConverter : IValueConverter
    {
        public object Convert (object value, Type targetType, object parameter, string language)
        {
            switch ((TunnelConnectionStatus)value)
            {
                case TunnelConnectionStatus.Connected:
                case TunnelConnectionStatus.Connecting:
                    return true;
                // case TunnelConnectionStatus.Disconnected:
                // case TunnelConnectionStatus.Disconnecting:
                default:
                    return false;
            }
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
