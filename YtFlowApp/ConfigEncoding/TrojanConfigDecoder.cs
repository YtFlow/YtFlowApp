using System;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    class TrojanConfigDecoder : IConfigDecoder
    {
        public IAdapterConfig Decode (string data)
        {
            Uri uri;
            try
            {
                uri = new Uri(data);
            }
            catch (FormatException)
            {
                return null;
            }
            if (!uri.IsAbsoluteUri)
            {
                return null;
            }

            var scheme = uri.Scheme;
            if (scheme != "trojan") return null;
            var trojanConfig = new TrojanConfig
            {
                ServerHost = uri.Host,
                ServerPort = uri.Port == -1 ? 443 : uri.Port,
                Password = uri.UserInfo
            };
            return trojanConfig;
        }
    }
}
