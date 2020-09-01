using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;

namespace YtFlow.Tasks.Hosted.Source
{
    public sealed class UrlAssociatedData
    {
        public TrafficUsage TrafficUsage { get; set; }
    }
    public sealed class UrlSource : IHostedConfigSource
    {
        private static readonly char[] KV_SEP = new char[] { ';' };
        private static readonly JsonSerializer JsonSerializer = JsonSerializer.CreateDefault();

        public string SourceType { get => "url"; set { } }

        public string Url { get; set; }

        private TrafficUsage GetTrafficUsageFromQuantumultXResponseHeader (string headerValue)
        {
            if (headerValue == null)
            {
                return null;
            }
            var dict = headerValue
                .Split(KV_SEP, StringSplitOptions.RemoveEmptyEntries)
                .Where(kv => kv.Contains('='))
                .Select(kv =>
            {
                var eqIndex = kv.IndexOf('=');
                return (kv.Substring(0, eqIndex).Trim(), kv.Substring(eqIndex + 1).Trim());
            }).ToDictionary(kv => kv.Item1, kv => kv.Item2);
            try
            {
                return new TrafficUsage()
                {
                    UploadUsed = long.Parse(dict["upload"]),
                    DownloadUsed = long.Parse(dict["download"]),
                    TrafficTotal = long.Parse(dict["total"]),
                    Expiry = DateTimeOffset.FromUnixTimeSeconds(long.Parse(dict["expire"])).UtcDateTime
                };
            }
            catch (Exception ex) when (
                ex is FormatException _
                || ex is OverflowException _
                || ex is KeyNotFoundException _)
            {
                return null;
            }
        }

        public string GetFileName ()
        {
            Uri uri;
            try
            {
                uri = new Uri(Url, UriKind.Absolute);
            }
            catch (FormatException)
            {
                return null;
            }

            return uri
                .GetComponents(UriComponents.Path, UriFormat.Unescaped)
                .Split('/')
                .LastOrDefault();
        }
        public IAsyncOperation<FetchResult> FetchAsync ()
        {
            return AsyncInfo.Run(async token =>
            {
                using (var msg = await HttpUtils.HttpClient
                    .GetAsync(new Uri(Url), Windows.Web.Http.HttpCompletionOption.ResponseHeadersRead)
                    .AsTask(token))
                {
                    msg.EnsureSuccessStatusCode();
                    TrafficUsage trafficUsage = null;
                    if (msg.Headers.TryGetValue("subscription-userinfo", out var subValue))
                    {
                        trafficUsage = GetTrafficUsageFromQuantumultXResponseHeader(subValue);
                    }
                    return new FetchResult()
                    {
                        SourceAssociatedData = new UrlAssociatedData() { TrafficUsage = trafficUsage },
                        Stream = await msg.Content.ReadAsInputStreamAsync().AsTask(token)
                    };
                }
            });
        }

        public object GetAssociatedDataFromObject (object obj)
        {
            if (!(obj is JObject json))
            {
                return obj;
            }
            UrlAssociatedData ret = new UrlAssociatedData();
            try
            {
                JsonSerializer.Populate(json.CreateReader(), ret);
                return ret;
            }
            catch (JsonException)
            {
                return null;
            }
        }
    }
}
