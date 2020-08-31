using System.Collections.Generic;

namespace YtFlow.App.Models
{
    public class SnapshotItemGroup
    {
        public string Name { get; set; }
        public IList<object> Items { get; set; }
    }
}
