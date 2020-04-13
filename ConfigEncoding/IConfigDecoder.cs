using System.Collections.Generic;
using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    interface IConfigDecoder
    {
        List<IAdapterConfig> Decode(string data);
    }
}
