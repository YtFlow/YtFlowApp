using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Storage.Streams;
using YtFlow.Tunnel.Config;

namespace YtFlow.Tasks.Hosted.Format
{
    public sealed class SsdAssociatedData
    {
        public string Airport { get; set; }
        public double? TrafficUsed { get; set; }
        public double? TrafficTotal { get; set; }
        // DateTime? is not a valid WinRT type
        // public DateTime? Expiry { get; set; }
        public object Expiry { get; set; }
        public string ExpiryString { get => Expiry?.ToString(); }
        public bool TrafficLimitAvailable { get => TrafficTotal != null && TrafficUsed != null; }
        public string TrafficLimit
        {
            get
            {
                if (TrafficTotal == null)
                {
                    return string.Empty;
                }
                string prefix = string.Empty;
                if (TrafficUsed != null)
                {
                    prefix = $"{TrafficTotal - TrafficUsed:0.##} GB available / ";
                }
                return $"{prefix}{TrafficTotal:0.##} GB in total";
            }
        }
        public double TrafficRemainingPercentage
        {
            get => TrafficLimitAvailable
                ? (TrafficTotal.Value - TrafficUsed.Value) / TrafficTotal.Value * 100.0
                : 0d;
        }
    }
    public sealed class Ssd : IHostedConfigFormat
    {
        private class SsdData
        {
            public class SsdServer
            {
                public string Server { get; set; }
                public ushort? Port { get; set; }
                public string Encryption { get; set; }
                public string Password { get; set; }
                public string Remarks { get; set; } = "New Server";
                // The ID field is completely useless.
                // public int Id { get; set; }
                // TODO: Other fields are omitted

            }
            public string Airport { get; set; }
            public ushort Port { get; set; }
            public string Encryption { get; set; }
            public string Password { get; set; }
            [JsonProperty(NamingStrategyType = typeof(SnakeCaseNamingStrategy))]
            public double? TrafficUsed { get; set; }
            [JsonProperty(NamingStrategyType = typeof(SnakeCaseNamingStrategy))]
            public double? TrafficTotal { get; set; }
            public DateTime? Expiry { get; set; }
            public List<SsdServer> Servers { get; set; } = new List<SsdServer>();
            // TODO: Some Optional and Extended fields are omitted
        }
        private const string PREFIX = "ssd://";
        private static readonly InvalidDataException INVALID_PREFIX = new InvalidDataException("Data stream does not start with ssd://");
        private static readonly JsonSerializerSettings SERIALIZER_SETTINGS = new JsonSerializerSettings()
        {
            NullValueHandling = NullValueHandling.Ignore,
            DateFormatString = "yyyy'-'MM'-'dd' 'HH':'mm':'ss"
        };

        public string FriendlyName { get => "SSD"; }
        public string FormatType { get => "ssd"; set { } }

        public IAsyncOperation<Snapshot> DecodeAsync (IInputStream inputStream)
        {
            return DecodeImpl(inputStream).AsAsyncOperation();
        }
        private static async Task<Snapshot> DecodeImpl (IInputStream inputStream)
        {
            string b64;
            // TODO: stream processing
            using (var reader = new StreamReader(inputStream.AsStreamForRead()))
            {
                // No way to pass a cancellation token in.
                // See https://github.com/dotnet/runtime/issues/20824
                b64 = await reader.ReadToEndAsync();
            }
            if (!b64.StartsWith("ssd://"))
            {
                throw INVALID_PREFIX;
            }

            var b612 = b64.Substring(PREFIX.Length).Replace('-', '+').Replace('_', '/');
            var json = Encoding.UTF8.GetString(System.Convert.FromBase64String(
                b612.PadRight(b612.Length + (4 - b612.Length % 4) % 4, '=')));
            var data = JsonConvert.DeserializeObject<SsdData>(json, SERIALIZER_SETTINGS);

            var servers = new List<IAdapterConfig>(data.Servers.Count);
            foreach (var server in data.Servers)
            {
                servers.Add(new ShadowsocksConfig
                {
                    ServerHost = server.Server,
                    ServerPort = server.Port ?? data.Port,
                    Method = server.Encryption ?? data.Encryption,
                    Password = server.Password ?? data.Password,
                    Name = server.Remarks
                });
            }
            return new Snapshot()
            {
                AdapterConfigs = servers,
                AssociatedData = new JObject
                {
                    { nameof(SsdData.Airport), data.Airport },
                    { nameof(SsdData.TrafficUsed), data.TrafficUsed },
                    { nameof(SsdData.TrafficTotal), data.TrafficTotal },
                    { nameof(SsdData.Expiry), data.Expiry },
                }
            };
        }

        public object GetAssociatedDataFromObject (object obj)
        {
            if (obj is JObject json)
            {
                return new SsdAssociatedData()
                {
                    Airport = (string)json[nameof(SsdData.Airport)],
                    TrafficUsed = (double?)json[nameof(SsdData.TrafficUsed)],
                    TrafficTotal = (double?)json[nameof(SsdData.TrafficTotal)],
                    Expiry = (DateTime?)json[nameof(SsdData.Expiry)],
                };
            }
            return null;
        }
    }
}
