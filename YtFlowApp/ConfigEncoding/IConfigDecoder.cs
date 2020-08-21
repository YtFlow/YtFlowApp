using YtFlow.Tunnel.Config;

namespace YtFlow.App.ConfigEncoding
{
    interface IConfigDecoder
    {
        IAdapterConfig Decode(string uri);
    }
}
