using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.RegularExpressions;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    internal class ShadowsocksConfigDecoder : IConfigDecoder
    {
        private static readonly Regex SchemeMatch = new Regex(@"ss://(?<base64>[A-Za-z0-9+-/=_]+)(?:#(?<tag>\S+))?", RegexOptions.IgnoreCase | RegexOptions.Compiled);

        private static readonly Regex InfoMatch = new Regex(@"^((?<method>.+?):(?<password>.*)@(?<host>.+?):(?<port>\d+?))$", RegexOptions.IgnoreCase | RegexOptions.Compiled);

        public List<IAdapterConfig> Decode(string data)
        {
            var uris = data.Split(new[] { '\r', '\n', ' ' }, StringSplitOptions.RemoveEmptyEntries).Select(x => x.Trim());
            var servers = new List<IAdapterConfig>();
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
                var name = WebUtility.UrlDecode(uri.GetComponents(UriComponents.Fragment, UriFormat.Unescaped));
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