using System;
using System.Net;
using System.Text;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    public static class ConfigEncoder
    {
        public static string Encode(this IAdapterConfig adpaterConfig)
        {
            switch (adpaterConfig)
            {
                case ShadowsocksConfig shadowsocksConfig:
                    return ShadowsocksConfigEncoder(shadowsocksConfig);
                case TrojanConfig trojanConfig:
                    return TrojanConfigEncoder(trojanConfig);
            }
            return "";
        }

        private static string ShadowsocksConfigEncoder(ShadowsocksConfig config)
        {
            string tag = string.Empty;
            var parts = $"{config.Method}:{config.Password}";
            var base64 = Convert.ToBase64String(Encoding.UTF8.GetBytes(parts));
            var websafeBase64 = base64.Replace('+', '-').Replace('/', '_').TrimEnd('=');
            var plugin = "";
            var url = $"{websafeBase64}@{config.ServerHost}:{config.ServerPort}/?plugin={WebUtility.UrlEncode(plugin)}";
            if (!string.IsNullOrEmpty(config.Name))
            {
                tag = $"#{WebUtility.UrlEncode(config.Name)}";
            }
            return $"ss://{url}{tag}";
        }

        private static string TrojanConfigEncoder(TrojanConfig config)
        {
            return $"trojan://{Uri.EscapeDataString(config.Password)}@{config.ServerHost}:{config.ServerPort}";
        }
    }
}