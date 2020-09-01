using System.ComponentModel;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.App.Models;
using YtFlow.Tasks.Hosted.Format;
using YtFlow.Tasks.Hosted.Source;

namespace YtFlow.App.Selectors
{
    class HostedConfigListItemAssociatedDataSelector : DataTemplateSelector, INotifyPropertyChanged
    {
        private static readonly PropertyChangedEventArgs SourceAssociatedDataPropertyChangedEventArgs
            = new PropertyChangedEventArgs(nameof(SourceAssociatedData));
        private static readonly PropertyChangedEventArgs FormatAssociatedDataPropertyChangedEventArgs
            = new PropertyChangedEventArgs(nameof(FormatAssociatedData));

        public IHostedConfigSource Source
        {
            get => source;
            set
            {
                source = value;
                if (snapshotTask is SnapshotTask task && task.Task.Status == TaskStatus.RanToCompletion)
                {
                    SourceAssociatedData = source.GetAssociatedDataFromObject(task.Task.Result?.SourceAssociatedData);
                }
                else
                {
                    SourceAssociatedData = null;
                }
                PropertyChanged?.Invoke(this, SourceAssociatedDataPropertyChangedEventArgs);
            }
        }
        public IHostedConfigFormat Format
        {
            get => format; set
            {
                format = value;
                if (snapshotTask is SnapshotTask task && task.Task.Status == TaskStatus.RanToCompletion)
                {
                    FormatAssociatedData = format.GetAssociatedDataFromObject(task.Task.Result?.FormatAssociatedData);
                }
                else
                {
                    FormatAssociatedData = null;
                }
                PropertyChanged?.Invoke(this, FormatAssociatedDataPropertyChangedEventArgs);
            }
        }
        public SnapshotTask SnapshotTask
        {
            get => snapshotTask; set
            {
                snapshotTask = value;
                if (source is IHostedConfigSource configSource && snapshotTask.Task.Status == TaskStatus.RanToCompletion)
                {
                    SourceAssociatedData = source.GetAssociatedDataFromObject(snapshotTask.Task.Result?.SourceAssociatedData);
                }
                else
                {
                    SourceAssociatedData = null;
                }
                PropertyChanged?.Invoke(this, SourceAssociatedDataPropertyChangedEventArgs);
                if (format is IHostedConfigFormat configFormat && snapshotTask.Task.Status == TaskStatus.RanToCompletion)
                {
                    FormatAssociatedData = format.GetAssociatedDataFromObject(snapshotTask.Task.Result?.FormatAssociatedData);
                }
                else
                {
                    FormatAssociatedData = null;
                }
                PropertyChanged?.Invoke(this, FormatAssociatedDataPropertyChangedEventArgs);
            }
        }
        public object FormatAssociatedData { get; set; }
        public object SourceAssociatedData { get; set; }
        private IHostedConfigFormat format;
        private IHostedConfigSource source;
        private SnapshotTask snapshotTask;
        public DataTemplate SsdTemplate { get; set; }
        public DataTemplate UrlTemplate { get; set; }
        public DataTemplate NullTemplate { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            switch (item)
            {
                case SsdAssociatedData _:
                    return SsdTemplate;
                case UrlAssociatedData u when u.TrafficUsage != null:
                    // Avoid InvalidCastException thrown from generated code
                    // for bindings when UrlAssociateData is not null but its
                    // TrafficUsage property is null.
                    // TODO: need further investigation
                    return UrlTemplate;
            }
            return NullTemplate;
        }
    }
}
