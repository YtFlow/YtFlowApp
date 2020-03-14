using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.Storage;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.Utils
{
    internal class ShadowsocksUtils
    {
        private static readonly Regex SchemeMatch =
    new Regex(@"ss://(?<base64>[A-Za-z0-9+-/=_]+)(?:#(?<tag>\S+))?", RegexOptions.IgnoreCase | RegexOptions.Compiled);

        private static readonly Regex InfoMatch =
            new Regex(@"^((?<method>.+?):(?<password>.*)@(?<host>.+?):(?<port>\d+?))$", RegexOptions.IgnoreCase | RegexOptions.Compiled);

        public static List<ShadowsocksConfig> GetServers(string ssUri)
        {
            var uris = ssUri.Split(new[] { '\r', '\n', ' ' }, StringSplitOptions.RemoveEmptyEntries).Select(x => x.Trim());
            var servers = new List<ShadowsocksConfig>();
            foreach (var uri in uris)
            {
                var config = DecodeUriToConfig(uri);
                if (config != null)
                {
                    servers.Add(config);
                }
            }
            return servers;
        }

        public static async Task SaveServersAsync(List<ShadowsocksConfig> configs)
        {
            var lastConfig = "";
            foreach (var config in configs)
            {
                if (string.IsNullOrEmpty(config.Path))
                {
                    var dir = await Utils.GetAdapterConfigDirectory();
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

        public static string EncodeConfigToUri(ShadowsocksConfig config)
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

        private static ShadowsocksConfig DecodeOldUrl(string url)
        {
            var match = SchemeMatch.Match(url);
            if (!match.Success)
                return null;
            var base64 = match.Groups["base64"].Value.TrimEnd('/');
            var tag = match.Groups["tag"].Value;
            var name = string.Empty;
            if (!string.IsNullOrEmpty(tag))
            {
                name = WebUtility.UrlDecode(tag);
            }
            var infos = InfoMatch.Match(Encoding.UTF8.GetString(Convert.FromBase64String(
                base64.PadRight(base64.Length + (4 - base64.Length % 4) % 4, '='))));
            if (!infos.Success)
                return null;
            return new ShadowsocksConfig
            {
                Name = name,
                Method = infos.Groups["method"].Value,
                Password = infos.Groups["password"].Value,
                ServerHost = infos.Groups["host"].Value,
                ServerPort = int.Parse(infos.Groups["port"].Value)
            };
        }

        private static ShadowsocksConfig DecodeUriToConfig(string ssUri)
        {
            if (string.IsNullOrEmpty(ssUri) || !ssUri.StartsWith("ss://", StringComparison.OrdinalIgnoreCase))
                return null;
            var config = DecodeOldUrl(ssUri);
            if (config == null)
            {
                var uri = new Uri(ssUri);
                var name = uri.GetComponents(UriComponents.Fragment, UriFormat.Unescaped);
                var host = uri.IdnHost;
                var port = uri.Port;
                var rawInfos = uri.GetComponents(UriComponents.UserInfo, UriFormat.Unescaped);
                var base64 = rawInfos.Replace('-', '+').Replace('_', '/');
                var infos = Encoding.UTF8.GetString(Convert.FromBase64String(
                    base64.PadRight(base64.Length + (4 - base64.Length % 4) % 4, '=')));
                string[] userInfoParts = infos.Split(new char[] { ':' }, 2);
                if (userInfoParts.Length != 2)
                {
                    return null;
                }
                var method = userInfoParts[0];
                var password = userInfoParts[1];
                return new ShadowsocksConfig
                {
                    Name = name,
                    ServerHost = host,
                    ServerPort = port,
                    Method = method,
                    Password = password
                };
            }
            else
            {
                return config;
            }
        }
    }
}