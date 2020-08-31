using Humanizer;
using System;
using Windows.UI.Xaml.Data;

namespace YtFlow.App.Converters
{
    class DateTimeOffsetHumanizer : IValueConverter
    {
        public object Convert (object value, Type targetType, object parameter, string language)
        {
            if (value is DateTimeOffset dto)
            {
                return dto.Humanize();
            }
            return string.Empty;
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
