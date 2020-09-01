using System;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Storage.Streams;
using YtFlow.Tasks.Hosted.Format;

namespace YtFlow.Tasks.Hosted.Source
{
    public sealed class FetchResult : IDisposable
    {
        public object SourceAssociatedData { get; set; }
        public IInputStream Stream { get; set; }

        public IAsyncOperation<Snapshot> DecodeAsync (IHostedConfigFormat format)
        {
            return AsyncInfo.Run(async cancellationToken =>
            {
                var snapshot = await format.DecodeAsync(Stream).AsTask(cancellationToken);
                snapshot.SourceAssociatedData = SourceAssociatedData;
                return snapshot;
            });
        }

        public void Dispose ()
        {
            Stream.Dispose();
        }
    }
}
