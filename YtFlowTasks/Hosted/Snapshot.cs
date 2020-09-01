using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using YtFlow.Tunnel.Config;

namespace YtFlow.Tasks.Hosted
{
    public sealed class Snapshot
    {
        [JsonIgnore]
        public string Path { get; set; }
        public DateTimeOffset? UpdatedAt { get; set; }
        public DateTimeOffset RetrievedAt { get; set; }
        [JsonConverter(typeof(AdapterIListConverter))]
        public IList<IAdapterConfig> AdapterConfigs { get; set; }
        public object SourceAssociatedData { get; set; }
        public object FormatAssociatedData { get; set; }

        public Snapshot ()
        {
            RetrievedAt = DateTimeOffset.Now;
        }
    }
}
