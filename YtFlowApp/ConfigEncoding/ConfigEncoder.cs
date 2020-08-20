using System;
using System.Net;
using System.Text;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    public static class ConfigEncoder
    {
        public static string Encode(this IAdapterConfig adapterConfig)
        {
            switch (adapterConfig)
            {
                case ShadowsocksConfig shadowsocksConfig:
                    return ShadowsocksConfigEncoder(shadowsocksConfig);
                case TrojanConfig trojanConfig:
                    return TrojanConfigEncoder(trojanConfig);
            }
            throw new NotSupportedException("Could not generate a URI of type " + adapterConfig.AdapterType);
        }

        /// <summary>
        /// Shadowsocks SIP002-compliant URI encoding
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        private static string ShadowsocksConfigEncoder(ShadowsocksConfig config)
        {
            var tag = string.Empty;
            var parts = $"{config.Method}:{config.Password}";
            var base64 = Convert.ToBase64String(Encoding.UTF8.GetBytes(parts));
            var websafeBase64 = base64.Replace('+', '-').Replace('/', '_').TrimEnd('=');
            var plugin = string.Empty; // Reserve for future
            if (!string.IsNullOrWhiteSpace(plugin))
            {
                plugin = "?plugin=" + WebUtility.UrlEncode(plugin);
            }
            var url = $"{websafeBase64}@{config.ServerHost}:{config.ServerPort}/{plugin}";
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