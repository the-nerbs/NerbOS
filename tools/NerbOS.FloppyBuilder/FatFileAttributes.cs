using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NerbOS.FloppyBuilder
{
    [Flags]
    enum FatFileAttributes
    {
        None = 0,

        ReadOnly  = (1 << 0),
        Hidden    = (1 << 1),
        System    = (1 << 2),
        VolumeId  = (1 << 3),
        Directory = (1 << 4),
        Archive   = (1 << 5),

        LongFileName = ReadOnly | Hidden | System | VolumeId,
    }
}
