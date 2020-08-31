using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using YtFlow.App.Models;

namespace YtFlow.App.Selectors
{
    public class HostedConfigListItemTaskStatusSelector : DataTemplateSelector
    {
        public DataTemplate RunningTemplate { get; set; }
        public DataTemplate FaultedTemplate { get; set; }
        public DataTemplate CompletionTemplate { get; set; }
        public DataTemplate NullTemplate { get; set; }
        protected override DataTemplate SelectTemplateCore (object item)
        {
            if (!(item is SnapshotTask obj))
            {
                return NullTemplate;
            }
            switch (obj.Task.Status)
            {
                case TaskStatus.Faulted:
                    return FaultedTemplate;
                case TaskStatus.RanToCompletion:
                    if (obj.Task.Result == null)
                    {
                        return NullTemplate;
                    }
                    else
                    {
                        return CompletionTemplate;
                    }
                default:
                    return RunningTemplate;
            }
        }

        protected override DataTemplate SelectTemplateCore (object item, DependencyObject container)
        {
            return SelectTemplateCore(item);
        }
    }
}
