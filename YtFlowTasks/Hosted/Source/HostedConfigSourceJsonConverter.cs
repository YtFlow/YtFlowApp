using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace YtFlow.Tasks.Hosted.Source
{
    internal class HostedConfigSourceJsonConverter : JsonConverter<IHostedConfigSource>
    {
        public override bool CanRead => true;
        public override bool CanWrite => true;
        public override IHostedConfigSource ReadJson (JsonReader reader, Type objectType, IHostedConfigSource existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var obj = JObject.Load(reader);
            var type = obj[nameof(IHostedConfigSource.SourceType)].Value<string>();
            var subReader = obj.CreateReader();
            switch (type)
            {
                case "url":
                    var url = new UrlSource();
                    serializer.Populate(subReader, url);
                    return url;
                case "file":
                    var file = new FileSource();
                    serializer.Populate(subReader, file);
                    return file;
            }
            return null;
        }

        public override void WriteJson (JsonWriter writer, IHostedConfigSource value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }
    }
}
