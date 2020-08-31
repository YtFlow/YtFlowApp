using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Windows.UI.Xaml.Data;
using YtFlow.App.Models;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Converters
{
    partial class SnapshotTaskToGroupConverter : IValueConverter
    {
        public object Convert (object value, Type targetType, object parameter, string language)
        {
            if (!(value is SnapshotTask snapshotTask)
                || snapshotTask.Task.Status != TaskStatus.RanToCompletion)
            {
                return Array.Empty<SnapshotItemGroup>();
            }
            var snapshot = snapshotTask.Task.Result;
            var groups = new List<SnapshotItemGroup>();
            if (snapshot?.AdapterConfigs is IList<IAdapterConfig> configs && configs.Count > 0)
            {
                groups.Add(new SnapshotItemGroup()
                {
                    Name = "Servers",
                    Items = configs.Cast<object>().ToList()
                });
            }
            return groups;
        }

        public object ConvertBack (object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
