using Newtonsoft.Json;
using YtFlow.Tasks.Hosted.Format;
using YtFlow.Tasks.Hosted.Source;

namespace YtFlow.Tasks.Hosted
{
    public sealed class HostedConfig
    {
        public string Name { get; set; }
        [JsonIgnore]
        public string Path { get; set; }
        [JsonConverter(typeof(HostedConfigSourceJsonConverter))]
        public IHostedConfigSource Source { get; set; }
        [JsonConverter(typeof(HostedConfigFormatJsonConverter))]
        public IHostedConfigFormat Format { get; set; }
    }
}
