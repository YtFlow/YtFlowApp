using Newtonsoft.Json;
using System;
using System.IO;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Core;

namespace YtFlow.Tasks.Hosted.Source
{
    public sealed class FileSource : IHostedConfigSource
    {
        private static readonly FileOpenPicker FileOpenPicker = new FileOpenPicker()
        {
            SuggestedStartLocation = PickerLocationId.Downloads
        };
        private static readonly InvalidOperationException WrongThreadException =
            new InvalidOperationException("Cannot update a snapshot with source File on a background thread.");
        private static readonly InvalidDataException NoFileException =
            new InvalidDataException("No file is chosen.");

        public FileSource () { }
        public FileSource (StorageFile preloadedFile)
        {
            PreloadedFile = preloadedFile;
            FileName = preloadedFile.Name;
        }

        [JsonIgnore]
        private StorageFile PreloadedFile;

        public string SourceType { get => "file"; set { } }

        public string FileName { get; set; }

        public IAsyncOperation<FetchResult> FetchAsync ()
        {
            if (CoreWindow.GetForCurrentThread() == null)
            {
                // In a background thread
                throw WrongThreadException;
            }
            return AsyncInfo.Run(async token =>
            {
                if (PreloadedFile == null)
                {
                    FileOpenPicker.FileTypeFilter.Clear();
                    if (Path.GetExtension(FileName) is string ext && !string.IsNullOrEmpty(ext))
                    {
                        FileOpenPicker.FileTypeFilter.Add(ext);
                    }
                    else
                    {
                        FileOpenPicker.FileTypeFilter.Add("*");
                    }
                    PreloadedFile = await FileOpenPicker.PickSingleFileAsync().AsTask(token);
                }
                if (PreloadedFile == null)
                {
                    throw NoFileException;
                }
                var stream = await PreloadedFile.OpenAsync(FileAccessMode.Read).AsTask(token);
                PreloadedFile = null;
                return new FetchResult() { Stream = stream, };
            });
        }

        public string GetFileName ()
        {
            return FileName;
        }

        public object GetAssociatedDataFromObject (object obj)
        {
            return null;
        }
    }
}
