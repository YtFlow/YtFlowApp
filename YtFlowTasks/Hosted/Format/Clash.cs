using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Storage.Streams;
using YamlDotNet.Serialization;
using YamlDotNet.Serialization.NamingConventions;
using YtFlow.Tunnel.Config;

namespace YtFlow.Tasks.Hosted.Format
{
    public sealed class Clash : IHostedConfigFormat
    {
        private class ClashData
        {
            public IList<Dictionary<string, object>> Proxies { get; set; }
        }
        private static readonly IDeserializer Deserializer = new DeserializerBuilder()
            .WithNamingConvention(HyphenatedNamingConvention.Instance)
            .IgnoreUnmatchedProperties()
            .Build();
        public string FormatType { get => "clash"; set { } }

        public string FriendlyName => "Clash";

        public IAsyncOperation<Snapshot> DecodeAsync (IInputStream inputStream)
        {
            return DecodeImpl(inputStream).AsAsyncOperation();
        }
        private static async Task<Snapshot> DecodeImpl (IInputStream inputStream)
        {
            string content;
            // TODO: stream processing
            using (var reader = new StreamReader(inputStream.AsStreamForRead()))
            {
                // No way to pass a cancellation token in.
                // See https://github.com/dotnet/runtime/issues/20824
                content = await reader.ReadToEndAsync();
            }
            var data = Deserializer.Deserialize<ClashData>(content);

            var servers = new List<IAdapterConfig>(data.Proxies.Count);
            foreach (var server in data.Proxies)
            {
                if (!server.TryGetValue("type", out var adapterTypeObj)
                    || !(adapterTypeObj is string adapterType))
                {
                    continue;
                }
                switch (adapterType)
                {
                    case "ss":
                        servers.Add(new ShadowsocksConfig()
                        {
                            ServerHost = (string)server["server"],
                            ServerPort = ushort.Parse(server["port"].ToString()),
                            Method = (string)server["cipher"],
                            Password = (string)server["password"],
                            Name = (string)server["name"]
                        });
                        break;
                    case "trojan":
                        servers.Add(new TrojanConfig()
                        {
                            ServerHost = (string)server["server"],
                            ServerPort = ushort.Parse(server["port"].ToString()),
                            Password = (string)server["password"],
                            Name = (string)server["name"],
                            AllowInsecure = server.TryGetValue("skip-cert-verify", out var allowInsecureObj)
                                && allowInsecureObj is bool allowInsecure && allowInsecure
                        });
                        break;
                    case "http":
                        string httpUsername = null, httpPassword = null;
                        if (server.TryGetValue("username", out var httpUsernameObj))
                        {
                            httpUsername = (string)httpUsernameObj;
                        }
                        if (server.TryGetValue("username", out var httpPasswordObj))
                        {
                            httpPassword = (string)httpPasswordObj;
                        }
                        servers.Add(new HttpConfig()
                        {
                            ServerHost = (string)server["server"],
                            ServerPort = ushort.Parse(server["port"].ToString()),
                            UserName = httpUsername,
                            Password = httpPassword,
                            Name = (string)server["name"]
                        });
                        break;
                }
            }

            return new Snapshot()
            {
                AdapterConfigs = servers,
            };
        }

        public object GetAssociatedDataFromObject (object obj)
        {
            return null;
        }
    }
}
