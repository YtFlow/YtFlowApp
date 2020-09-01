using Newtonsoft.Json;

namespace YtFlow.Tasks
{
    public sealed class TrafficUsage
    {
        public long UploadUsed { get; set; } = 0L;
        public long DownloadUsed { get; set; } = 0L;
        [JsonIgnore]
        public long TrafficUsed
        {
            get => UploadUsed + DownloadUsed; set
            {
                DownloadUsed = value;
                UploadUsed = 0L;
            }
        }
        public long TrafficTotal { get; set; }
        public object Expiry { get; set; }
        [JsonIgnore]
        public string ExpiryString { get => Expiry.ToString(); }
        [JsonIgnore]
        public string TrafficLimit => $"{(TrafficTotal - TrafficUsed) / 1073741824d:0.##} GB / {TrafficTotal / 1073741824d:0.##} GB";
        [JsonIgnore]
        public double TrafficRemainingPercentage => 100 - TrafficUsed / (double)TrafficTotal * 100.0;
    }
}
