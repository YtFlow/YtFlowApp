using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Storage;
using YtFlow.App.ConfigEncoding;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Utils
{
    internal class ConfigUtils
    {
        public static async Task SaveServersAsync(List<IAdapterConfig> configs)
        {
            var lastConfig = "";
            foreach (var config in configs)
            {
                if (string.IsNullOrEmpty(config.Path))
                {
                    var dir = await GetAdapterConfigDirectory();
                    var file = await dir.CreateFileAsync(Guid.NewGuid().ToString() + ".json", CreationCollisionOption.GenerateUniqueName);
                    config.Path = file.Path;
                }
                config.SaveToFile(config.Path);
                lastConfig = config.Path;
            }
            if (!string.IsNullOrEmpty(lastConfig))
            {
                AdapterConfig.SetDefaultConfigFilePath(lastConfig);
            }
        }

        public static IAsyncOperation<StorageFolder> GetAdapterConfigDirectory()
        {
            return ApplicationData.Current.RoamingFolder.CreateFolderAsync("configs", CreationCollisionOption.OpenIfExists);
        }

        public static List<IAdapterConfig> GetServers(string data)
        {
            var decoders = new List<IConfigDecoder> { new ShadowsocksConfigDecoder(), new TrojanConfigDecoder() };
            foreach (var decoder in decoders)
            {
                var configs = decoder.Decode(data);
                if (configs != null && configs.Count > 0)
                {
                    return configs;
                }
            }
            return new List<IAdapterConfig>();
        }
    }
}