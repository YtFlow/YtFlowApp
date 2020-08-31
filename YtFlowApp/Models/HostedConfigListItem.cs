using System;
using System.ComponentModel;
using System.Threading.Tasks;
using Windows.Storage;
using YtFlow.App.Utils;
using YtFlow.Tasks.Hosted;

namespace YtFlow.App.Models
{
    /// <summary>
    /// A wrapper for Task<Snapshot> to use in XAML
    /// </summary>
    internal class SnapshotTask
    {
        public Task<Snapshot> Task;
    }
    internal class HostedConfigListItem : INotifyPropertyChanged
    {
        private static readonly Task<Snapshot> ForeverTask = new TaskCompletionSource<Snapshot>().Task;
        private static readonly PropertyChangedEventArgs HostedConfigChangedEventArgs
            = new PropertyChangedEventArgs(nameof(HostedConfig));
        private static readonly PropertyChangedEventArgs LoadSnapshotTaskChangedEventArgs
            = new PropertyChangedEventArgs(nameof(LoadSnapshotTask));

        private Task<Snapshot> LastSucceededTask = ForeverTask;
        public HostedConfig HostedConfig { get; set; }
        public SnapshotTask LoadSnapshotTask { get; private set; } = new SnapshotTask()
        {
            Task = ForeverTask
        };

        public HostedConfigListItem (HostedConfig config)
        {
            HostedConfig = config;
            _ = StartLoadTask(HostedUtils.GetSnapshotFromHostConfigAsync(config));
        }
        public HostedConfigListItem (HostedConfig config, Snapshot snapshot)
        {
            HostedConfig = config;
            LoadSnapshotTask.Task = LastSucceededTask = Task.FromResult(snapshot);
        }

        private void TriggerUpdate (PropertyChangedEventArgs args)
        {
            PropertyChanged?.Invoke(this, args);
        }

        private async Task StartLoadTask (Task<Snapshot> task)
        {
            try
            {
                await task;
                LastSucceededTask = task;
            }
            catch (Exception) { }
            finally
            {
                LoadSnapshotTask = new SnapshotTask() { Task = task };
                TriggerUpdate(LoadSnapshotTaskChangedEventArgs);
            }
        }

        public async Task Rename (string newName)
        {
            HostedConfig.Name = newName;
            await HostedUtils.SaveHostedConfigAsync(HostedConfig);
            TriggerUpdate(HostedConfigChangedEventArgs);
        }

        public void TriggerTimeUpdate ()
        {
            TriggerUpdate(LoadSnapshotTaskChangedEventArgs);
        }

        public async Task DeleteAsync ()
        {
            var snapshot = await LoadSnapshotTask.Task;
            if (snapshot != null)
            {
                var snapshotFile = await StorageFile.GetFileFromPathAsync(snapshot.Path);
                await snapshotFile.DeleteAsync();
                TriggerUpdate(LoadSnapshotTaskChangedEventArgs);
            }
            var file = await StorageFile.GetFileFromPathAsync(HostedConfig.Path);
            await file.DeleteAsync();
        }

        public Task<Snapshot> UpdateAsync ()
        {
            if (!LoadSnapshotTask.Task.IsCompleted)
            {
                return LoadSnapshotTask.Task;
            }
            Snapshot oldSnapshot = null;
            if (LoadSnapshotTask.Task.Status == TaskStatus.RanToCompletion)
            {
                oldSnapshot = LoadSnapshotTask.Task.Result;
            }
            var task = HostedUtils.UpdateSnapshotAsync(HostedConfig, oldSnapshot);
            LoadSnapshotTask = new SnapshotTask() { Task = task };
            TriggerUpdate(LoadSnapshotTaskChangedEventArgs);
            _ = StartLoadTask(task);
            return task;
        }

        public void DismissError ()
        {
            LoadSnapshotTask = new SnapshotTask() { Task = LastSucceededTask };
            TriggerUpdate(LoadSnapshotTaskChangedEventArgs);
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
