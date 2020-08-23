using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.Provider;
using YtFlow.App.ConfigEncoding;
using YtFlow.App.Models;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Utils
{
    internal static class ConfigUtils
    {
        public static readonly Task<StorageFolder> AdapterConfigFolderTask =
            ApplicationData.Current.RoamingFolder.CreateFolderAsync("configs", CreationCollisionOption.OpenIfExists).AsTask();
        private static readonly List<IConfigDecoder> decoders = new List<IConfigDecoder>
        {
            new ShadowsocksConfigDecoder(),
            new TrojanConfigDecoder()
        };

        public static async Task<StorageFile> SaveServerAsync (IAdapterConfig config)
        {
            StorageFile file;
            if (string.IsNullOrEmpty(config.Path))
            {
                config.Name = string.IsNullOrEmpty(config.Name) ? $"New {config.AdapterType} Config" : config.Name;
                var dir = await AdapterConfigFolderTask;
                file = await dir.CreateFileAsync(Guid.NewGuid().ToString() + ".json", CreationCollisionOption.GenerateUniqueName);
                config.Path = file.Path;
            }
            else
            {
                file = await StorageFile.GetFileFromPathAsync(config.Path);
            }
            CachedFileManager.DeferUpdates(file);
            await config.SaveToFileAsync(config.Path);
            return file;
        }

        public static (List<IAdapterConfig> servers, List<string> unrecognized) DecodeServersFromLinks (string lines)
        {
            var ret = new List<IAdapterConfig>();
            var unrecognized = new List<string>();
            using (var reader = new StringReader(lines))
            {
                while (reader.ReadLine() is string line)
                {
                    if (decoders
                        .Select(decoder => decoder.Decode(line.Trim()))
                        .Where(c => c != null)
                        .FirstOrDefault() is IAdapterConfig config)
                    {
                        ret.Add(config);
                    }
                    else
                    {
                        unrecognized.Add(line);
                    }
                }
            }
            return (ret, unrecognized);
        }

        public static async Task<LinkImportResult> ImportLinksAsync (string links, Action<double> onProgress = null)
        {
            var (servers, unrecognized) = DecodeServersFromLinks(links);
            int saved = 0, failed = 0;
            var errors = new Dictionary<string, string>();
            var files = new List<StorageFile>(servers.Count);
            await Task.WhenAll(servers.Select(async server =>
            {
                try
                {
                    files.Add(await SaveServerAsync(server));
                    saved++;
                }
                catch (Exception ex)
                {
                    failed++;
                    errors[ex.Message] = ex.ToString();
                }
                finally
                {
                    onProgress?.Invoke((saved + failed) / (double)servers.Count * 100.0);
                }
            }));
            return new LinkImportResult()
            {
                SavedCount = saved,
                FailedCount = failed,
                UnrecognizedLines = unrecognized,
                Files = files,
                Errors = errors
            };
        }

        public static Task BatchCompleteUpdates (this List<StorageFile> files)
        {
            var tasks = new List<Task<FileUpdateStatus>>(files.Count);
            foreach (var file in files)
            {
                tasks.Add(CachedFileManager.CompleteUpdatesAsync(file).AsTask());
            }
            return Task.WhenAll(tasks);
        }
    }
}