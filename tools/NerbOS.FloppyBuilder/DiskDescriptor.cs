using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NerbOS.FloppyBuilder
{
    class DiskDescriptor
    {
        public const int SectorSize = 512;
        public const int FatSectors = 9;
        public const int RootDirSectors = 2;


        public int NumReservedSectors { get; set; }
        public int NextUnusedCluster { get; set; } = 2;
        public int NextUnusedRootEntry { get; set; } = 0;

        public int Fat0Offset
        {
            get { return NumReservedSectors * SectorSize; }
        }

        public int Fat1Offset
        {
            get { return Fat0Offset + FatSectors * SectorSize; }
        }


        public int RootDirOffset
        {
            get { return Fat1Offset + FatSectors * SectorSize; }
        }


        private int FirstDataSector
        {
            get { return NumReservedSectors + 2 * FatSectors + RootDirSectors; }
        }

        public int NextUnusedSector
        {
            get
            {
                return FirstDataSector + (NextUnusedCluster - 2);
            }
        }
    }
}
