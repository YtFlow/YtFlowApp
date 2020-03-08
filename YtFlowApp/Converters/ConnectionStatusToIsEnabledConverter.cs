using System;
using Windows.UI.Xaml.Data;
using YtFlow.App.Models;

namespace YtFlow.App.Converters
{
    class ConnectionStatusToIsEnabledConverter : IValueConverter
    {
        public object Convert (object value, Type targetType, object parameter, string language)
        {
            switch ((TunnelConnectionStatus)value)
            {
                case TunnelConnectionStatus.Disconnected:
                case TunnelConnectionStatus.Connected:
                    return true;
                // case VpnConnectionStatus.Connecting:
                // case VpnConnectionStatus.Disconnecting:
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
