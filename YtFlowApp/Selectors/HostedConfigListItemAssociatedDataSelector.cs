using System.ComponentModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.App.Models;
using YtFlow.Tasks.Hosted.Format;

namespace YtFlow.App.Selectors
{
    class HostedConfigListItemAssociatedDataSelector : DataTemplateSelector, INotifyPropertyChanged
    {
        private static readonly PropertyChangedEventArgs AssociatedDataPropertyChangedEventArgs
            = new PropertyChangedEventArgs(nameof(AssociatedData));

        public IHostedConfigFormat Format
        {
            get => format; set
            {
                format = value;
                if (snapshotTask is SnapshotTask task && task.Task.IsCompletedSuccessfully)
                {
                    AssociatedData = format.GetAssociatedDataFromObject(task.Task.Result?.AssociatedData);
                }
                else
                {
                    AssociatedData = null;
                }
                PropertyChanged?.Invoke(this, AssociatedDataPropertyChangedEventArgs);
            }
        }
        public SnapshotTask SnapshotTask
        {
            get => snapshotTask; set
            {
                snapshotTask = value;
                if (format is IHostedConfigFormat configFormat && snapshotTask.Task.IsCompletedSuccessfully)
                {
                    AssociatedData = format.GetAssociatedDataFromObject(snapshotTask.Task.Result?.AssociatedData);
                }
                else
                {
                    AssociatedData = null;
                }
                PropertyChanged?.Invoke(this, AssociatedDataPropertyChangedEventArgs);
            }
        }
        public object AssociatedData { get; set; }
        private IHostedConfigFormat format;
        private SnapshotTask snapshotTask;
        public DataTemplate SsdTemplate { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        protected override DataTemplate SelectTemplateCore (object item)
        {
            if (AssociatedData == null)
            {
                return null;
            }
            if (format is Ssd)
            {
                return SsdTemplate;
            }
            return base.SelectTemplateCore(item);
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
