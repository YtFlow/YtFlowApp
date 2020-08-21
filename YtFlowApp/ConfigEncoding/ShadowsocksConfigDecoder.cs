using System;
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

        private static ShadowsocksConfig DecodeOldUrl (string url)
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
            Match infos;
            try
            {
                infos = InfoMatch.Match(Encoding.UTF8.GetString(Convert.FromBase64String(
                    base64.PadRight(base64.Length + (4 - base64.Length % 4) % 4, '='))));
            }
            catch (Exception)
            {
                return null;
            }
            if (!infos.Success
                || !int.TryParse(infos.Groups["port"].Value, out var port))
            {
                return null;
            }
            return new ShadowsocksConfig
            {
                Name = name,
                Method = infos.Groups["method"].Value,
                Password = infos.Groups["password"].Value,
                ServerHost = infos.Groups["host"].Value,
                ServerPort = port,
            };
        }

        public IAdapterConfig Decode (string link)
        {
            try
            {
                return DecodeImpl(link);
            }
            catch (FormatException)
            {
                return null;
            }
        }
        /// <summary>
        /// Decode a Shadowsocks server aggressively
        /// </summary>
        /// <param name="link">URI</param>
        /// <exception cref="FormatException">
        ///     The link is not a valid URI,
        ///     or the user info part is not a valid Base64 string,
        ///     or the user info part contains invalid Unicode code points.
        /// </exception>
        /// <returns>A Shadowsocks server for a valid URI, otherwise null.</returns>
        private IAdapterConfig DecodeImpl (string link)
        {
            if (string.IsNullOrEmpty(link) || !link.StartsWith("ss://", StringComparison.OrdinalIgnoreCase))
                return null;
            var config = DecodeOldUrl(link);
            if (config == null)
            {
                var uri = new Uri(link);
                if (!uri.IsAbsoluteUri)
                {
                    return null;
                }
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