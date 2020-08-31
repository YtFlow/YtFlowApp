using Windows.Foundation;
using Windows.Storage.Streams;

namespace YtFlow.Tasks.Hosted.Format
{
    public interface IHostedConfigFormat
    {
        string FormatType { get; set; }
        string FriendlyName { get; }
        IAsyncOperation<Snapshot> DecodeAsync (IInputStream inputStream);
        object GetAssociatedDataFromObject (object obj);
    }
}
