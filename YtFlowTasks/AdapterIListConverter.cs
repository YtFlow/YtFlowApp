using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using YtFlow.Tunnel.Config;

namespace YtFlow.Tasks
{
    internal class AdapterIListConverter : JsonConverter<IList<IAdapterConfig>>
    {
        public override bool CanRead => true;
        public override bool CanWrite => true;
        public override IList<IAdapterConfig> ReadJson (JsonReader reader, Type objectType, IList<IAdapterConfig> existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var jsonList = JArray.Load(reader);
            var objList = new List<IAdapterConfig>(jsonList.Count);
            foreach (var json in jsonList)
            {
                var jsonReader = json.CreateReader();
                var adapterType = json[nameof(IAdapterConfig.AdapterType)].Value<string>();
                switch (adapterType)
                {
                    case "shadowsocks":
                        var ss = new ShadowsocksConfig();
                        serializer.Populate(jsonReader, ss);
                        objList.Add(ss);
                        break;
                    case "http":
                        var http = new HttpConfig();
                        serializer.Populate(jsonReader, http);
                        objList.Add(http);
                        break;
                    case "trojan":
                        var trojan = new TrojanConfig();
                        serializer.Populate(jsonReader, trojan);
                        objList.Add(trojan);
                        break;
                }
            }
            return objList;
        }

        public override void WriteJson (JsonWriter writer, IList<IAdapterConfig> value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }
    }
}
