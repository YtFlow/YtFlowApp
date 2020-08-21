using System;
using System.Collections.Generic;
using System.Linq;
using Windows.Storage;

namespace YtFlow.App.Models
{
    struct LinkImportResult
    {
        public int SavedCount;
        public int FailedCount;
        public List<string> UnrecognizedLines;
        public List<StorageFile> Files;
        public Dictionary<string, string> Errors;

        public string GenerateMessage ()
        {
            string errorMessage = string.Empty, unrecognizedMessage = string.Empty;
            if (Errors.Count > 0)
            {
                errorMessage = $@"
First {Math.Min(Errors.Count, 3)} types of errors:
{string.Join(Environment.NewLine, Errors.Values.Take(3))}";
            }
            if (UnrecognizedLines.Count > 0)
            {
                unrecognizedMessage = $@"
First {Math.Min(UnrecognizedLines.Count, 3)} unrecognized lines:
{string.Join(Environment.NewLine,
    UnrecognizedLines
        .Select(s => s.Substring(0, Math.Min(s.Length, 200)))
        .Take(3))}";
            }

            return $"Saved {SavedCount} configs" +
                            (UnrecognizedLines.Count > 0 ? $", {UnrecognizedLines.Count} lines uncognized" : string.Empty) +
                            (Errors.Count > 0 ? $", {FailedCount} failed" : string.Empty) +
                            $".{errorMessage}{unrecognizedMessage}";
        }
    }
}
