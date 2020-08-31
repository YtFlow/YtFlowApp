using System;
using System.Text;
using Windows.UI.Xaml.Data;
using YtFlow.Tasks.Hosted;

namespace YtFlow.App.Converters
{
    class SnapshotToDescriptionConverter : IValueConverter
    {
        private static readonly char[] trailingChars = new char[] { ' ', '\r', '\n', ',' };

        public string Separator { get; set; } = Environment.NewLine;

        public object Convert (object value, Type targetType, object parameter, string language)
        {
            if (!(value is Snapshot snapshot))
            {
                return "Unknown";
            }
            var sb = new StringBuilder();
            if (snapshot.AdapterConfigs.Count > 0)
            {
                sb.Append(snapshot.AdapterConfigs.Count.ToString());
                sb.Append(" servers");
                sb.Append(Separator);
            }
            var str = sb.ToString().TrimEnd(trailingChars);
            if (string.IsNullOrEmpty(str))
            {
                return "Empty";
            }
            else
            {
                return str;
            }
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
