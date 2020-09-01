using System;
using System.IO;
using System.Threading.Tasks;
using Windows.Storage;
using YtFlow.Tasks.Hosted;

namespace YtFlow.App.Utils
{
    internal static class HostedUtils
    {
        public static readonly Task<StorageFolder> HostedConfigFolderTask =
            ApplicationData.Current.RoamingFolder.CreateFolderAsync("hosted", CreationCollisionOption.OpenIfExists).AsTask();
        public static readonly Task<StorageFolder> SnapshotFolderTask =
            ApplicationData.Current.LocalCacheFolder.CreateFolderAsync("snapshot", CreationCollisionOption.OpenIfExists).AsTask();

        public static async Task<HostedConfig> GetHostedConfigAsync (IStorageFile file)
        {
            var config = await IoUtils.DeserializeFromFileAsync<HostedConfig>(file, default);
            config.Path = file.Path;
            return config;
        }

        public static async Task<Snapshot> GetSnapshotFromHostConfigAsync (HostedConfig config)
        {
            var fileName = Path.GetFileName(config.Path);
            StorageFile file;
            try
            {
                file = await (await SnapshotFolderTask).GetFileAsync(fileName);
                var snapshot = await IoUtils.DeserializeFromFileAsync<Snapshot>(file, default);
                snapshot.Path = file.Path;
                return snapshot;
            }
            catch (FileNotFoundException)
            {
                return null;
            }
        }

        public static async Task<StorageFile> SaveHostedConfigAsync (HostedConfig config)
        {
            var file = await IoUtils.SaveUniqueObjAsync(config.Path, config, await HostedConfigFolderTask);
            config.Path = file.Path;
            return file;
        }
        public static async Task<StorageFile> SaveSnapshotAsync (Snapshot snapshot, HostedConfig config)
        {
            var fileName = Path.GetFileName(config.Path);
            var file = await (await SnapshotFolderTask).CreateFileAsync(fileName, CreationCollisionOption.OpenIfExists);
            await IoUtils.SaveObjAsync(snapshot, file);
            snapshot.Path = file.Path;
            return file;
        }

        public static async Task<Snapshot> UpdateSnapshotAsync (HostedConfig hostedConfig, Snapshot oldSnapshot)
        {
            Snapshot newSnapshot;
            using (var source = await hostedConfig.Source.FetchAsync())
            {
                newSnapshot = await source.DecodeAsync(hostedConfig.Format);
            }
            var newSnapshotFile = await (await SnapshotFolderTask).CreateFileAsync(Path.GetFileName(hostedConfig.Path), CreationCollisionOption.ReplaceExisting);
            await IoUtils.SerializeToFilePathAsync(newSnapshotFile, newSnapshot, default);
            newSnapshot.Path = newSnapshotFile.Path;
            return newSnapshot;
        }
    }
}
