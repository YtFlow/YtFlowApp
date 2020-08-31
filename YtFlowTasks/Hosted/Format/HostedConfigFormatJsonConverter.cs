using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace YtFlow.Tasks.Hosted.Format
{
    internal class HostedConfigFormatJsonConverter : JsonConverter<IHostedConfigFormat>
    {
        public override bool CanRead => true;
        public override bool CanWrite => true;

        public override IHostedConfigFormat ReadJson (JsonReader reader, Type objectType, IHostedConfigFormat existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var obj = JObject.Load(reader);
            var format = obj[nameof(IHostedConfigFormat.FormatType)].Value<string>();
            switch (format)
            {
                case "ssd":
                    var ssd = new Ssd();
                    serializer.Populate(obj.CreateReader(), ssd);
                    return ssd;
                case "clash":
                    var clash = new Clash();
                    serializer.Populate(obj.CreateReader(), clash);
                    return clash;
            }
            return null;
        }

        public override void WriteJson (JsonWriter writer, IHostedConfigFormat value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }
    }
}
