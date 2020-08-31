using Windows.ApplicationModel;
using Windows.Web.Http;
using Windows.Web.Http.Headers;

namespace YtFlow.Tasks
{
    internal static class HttpUtils
    {
        private static HttpClient GetHttpClient ()
        {
            var client = new HttpClient();
            var ver = Package.Current.Id.Version;
            client.DefaultRequestHeaders.UserAgent.Add(
                new HttpProductInfoHeaderValue("YtFlow", $"{ver.Major}.{ver.Minor}.{ver.Build}.{ver.Revision}"));
            return client;
        }

        public static HttpClient HttpClient { get; } = GetHttpClient();
    }
}
