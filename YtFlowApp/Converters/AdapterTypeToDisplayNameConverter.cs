using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Data;

namespace YtFlow.App.Converters
{
    class AdapterTypeToDisplayNameConverter : IValueConverter
    {
        private static Dictionary<string, string> adapterTypeToDisplayName = new Dictionary<string, string>()
        {
            { "shadowsocks", "Shadowsocks" },
            { "http", "HTTP" },
            { "trojan", "Trojan" }
        };
        public object Convert (object value, Type targetType, object parameter, string language)
        {
            var str = value as string;
            if (string.IsNullOrWhiteSpace(str))
            {
                return value;
            }
            if (adapterTypeToDisplayName.TryGetValue(str, out var ret))
            {
                return ret;
            }
            else
            {
                return string.Empty;
            }

        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
