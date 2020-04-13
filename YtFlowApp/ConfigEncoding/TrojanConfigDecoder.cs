using System;
using System.Collections.Generic;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    class TrojanConfigDecoder : IConfigDecoder
    {
        public List<IAdapterConfig> Decode(string data)
        {
            var configs = new List<IAdapterConfig>();
            var config = DecodeUriToConfig(data);
            if (config != null) configs.Add(config);
            return configs;
        }

        private TrojanConfig DecodeUriToConfig(string data)
        {
            Uri uri;
            try
            {
                uri = new Uri(data);
            }
            catch (UriFormatException)
            {
                return null;
            }
            var scheme = uri.Scheme;
            if (scheme != "trojan") return null;
            var trojanConfig = new TrojanConfig
            {
                ServerHost = uri.Host,
                ServerPort = uri.Port,
                Password = uri.UserInfo
            };
            return trojanConfig;
        }
    }
}
