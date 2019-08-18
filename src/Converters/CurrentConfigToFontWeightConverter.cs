using System;
using Windows.UI.Text;
using Windows.UI.Xaml.Data;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Converters
{
    class CurrentConfigToFontWeightConverter : IValueConverter
    {
        public object Convert (object configPath, Type targetType, object parameter, string language)
        {
            if (AdapterConfig.GetDefaultConfigFilePath() == configPath as string)
            {
                return FontWeights.Bold;
            }
            return FontWeights.Normal;
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
