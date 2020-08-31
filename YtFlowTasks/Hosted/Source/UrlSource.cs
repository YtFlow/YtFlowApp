using System;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Storage.Streams;

namespace YtFlow.Tasks.Hosted.Source
{
    public sealed class UrlSource : IHostedConfigSource
    {
        public string SourceType { get => "url"; set { } }

        public string Url { get; set; }

        public string GetFileName ()
        {
            Uri uri;
            try
            {
                uri = new Uri(Url, UriKind.Absolute);
            }
            catch (FormatException)
            {
                return null;
            }

            return uri
                .GetComponents(UriComponents.Path, UriFormat.Unescaped)
                .Split('/')
                .LastOrDefault();
        }
        public IAsyncOperation<IInputStream> FetchAsync ()
        {
            return AsyncInfo.Run(async token =>
            {
                using (var msg = await HttpUtils.HttpClient
                    .GetAsync(new Uri(Url), Windows.Web.Http.HttpCompletionOption.ResponseHeadersRead)
                    .AsTask(token))
                {
                    msg.EnsureSuccessStatusCode();
                    return await msg.Content.ReadAsInputStreamAsync().AsTask(token);
                }
            });
        }
    }
}
