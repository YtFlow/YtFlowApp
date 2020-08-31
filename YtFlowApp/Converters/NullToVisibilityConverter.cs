using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Data;

namespace YtFlow.App.Converters
{
    class NullToVisibilityConverter : IValueConverter
    {
        public bool Reverse { get; set; } = false;

        public object Convert (object value, Type targetType, object parameter, string language)
        {
            return (Reverse ^ (value == null))
                ? Visibility.Collapsed
                : Visibility.Visible;
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
