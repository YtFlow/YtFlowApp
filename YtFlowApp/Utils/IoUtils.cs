using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.Provider;

namespace YtFlow.App.Utils
{
    internal static class IoUtils
    {
        private static readonly JsonSerializer serializer = JsonSerializer.CreateDefault();
        private static readonly UTF8Encoding UTF8Encoding = new UTF8Encoding(false);

        public static async Task SerializeToFilePathAsync<T> (IStorageFile file, T obj, CancellationToken cancellationToken)
        {
            using (var stream = new MemoryStream())
            using (var writer = new StreamWriter(stream, UTF8Encoding))
            {
                serializer.Serialize(writer, obj);
                writer.Flush();
                stream.Seek(0, SeekOrigin.Begin);
                await FileIO
                    .WriteBufferAsync(file, stream.GetWindowsRuntimeBuffer(0, (int)stream.Length))
                    .AsTask(cancellationToken);
            }
        }

        public static async Task<T> DeserializeFromFileAsync<T> (IStorageFile file, CancellationToken cancellationToken)
        {
            var textContent = await FileIO.ReadTextAsync(file).AsTask(cancellationToken);
            using (var reader = new StringReader(textContent))
            {
                return (T)serializer.Deserialize(reader, typeof(T));
            }
        }

        public static async Task<StorageFile> SaveUniqueObjAsync<T> (string path, T obj, StorageFolder folder)
        {
            StorageFile file;
            if (string.IsNullOrEmpty(path))
            {
                file = await folder.CreateFileAsync(Guid.NewGuid().ToString() + ".json", CreationCollisionOption.GenerateUniqueName);
            }
            else
            {
                file = await StorageFile.GetFileFromPathAsync(path);
            }
            await SaveObjAsync(obj, file);
            return file;
        }

        public static async Task SaveObjAsync<T> (T obj, StorageFile file)
        {
            CachedFileManager.DeferUpdates(file);
            await SerializeToFilePathAsync(file, obj, default);
        }

        public static Task BatchCompleteUpdates (this List<StorageFile> files)
        {
            var tasks = new List<Task<FileUpdateStatus>>(files.Count);
            foreach (var file in files)
            {
                tasks.Add(CachedFileManager.CompleteUpdatesAsync(file).AsTask());
            }
            return Task.WhenAll(tasks);
        }
    }
}
