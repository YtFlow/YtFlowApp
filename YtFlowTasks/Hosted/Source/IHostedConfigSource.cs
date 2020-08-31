using Windows.Foundation;
using Windows.Storage.Streams;

namespace YtFlow.Tasks.Hosted.Source
{
    public interface IHostedConfigSource
    {
        string SourceType { get; set; }
        string GetFileName ();
        IAsyncOperation<IInputStream> FetchAsync ();
    }
}
