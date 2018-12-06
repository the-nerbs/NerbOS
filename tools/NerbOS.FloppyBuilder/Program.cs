using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using static NerbOS.FloppyBuilder.DiskDescriptor;

namespace NerbOS.FloppyBuilder
{
    class Program
    {
        const int FloppySize = 1440 * 1024;
        const int RootDirSectors = 33 - 19;
        const int DirectoryEntrySize = 32;
        const int NRootDirectoryEntries = (RootDirSectors * SectorSize) / DirectoryEntrySize;


        static CommandLine cmd;
        static List<SourceInfo> sourceFiles = new List<SourceInfo>();
        static MemoryStream image;
        static DiskDescriptor diskDesc = new DiskDescriptor();


        static void Main(string[] args)
        {
            if (CommandLine.TryParse(args, out cmd))
            {
                try
                {
                    CollectSourceFiles();

                    CreateImage();
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Error : " + ex.Message);
                }
            }
            else
            {
                PrintUsage();
            }
        }


        static void PrintUsage()
        {
            Console.WriteLine(@"
Options:
boot <path>         Path to the boot sector image.
in <path>           Path to a directory whose contents to place on the floppy.
out <path>          Path to the output image file.

Additional unnamed arguments refer to individual files to include in the image.
");
        }


        static void CollectSourceFiles()
        {
            foreach (var item in cmd.SourceFiles)
            {
                try
                {
                    var file = new FileInfo(item);
                    if (file.Exists)
                    {
                        sourceFiles.Add(new SourceInfo(file));
                    }
                    else
                    {
                        Console.WriteLine(
                            $"Warning : source file '{item}' does not exist."
                        );
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine(
                        $"Warning : source file '{item}' is not a valid file path."
                    );
                }
            }

            if (!string.IsNullOrEmpty(cmd.SourceDir))
            {
                foreach (string path in Directory.EnumerateFileSystemEntries(cmd.SourceDir))
                {
                    sourceFiles.Add(new SourceInfo(path));
                }
            }
        }


        static void CreateImage()
        {
            image = new MemoryStream(new byte[FloppySize], writable: true);
            diskDesc.NumReservedSectors = 1;

            if (!string.IsNullOrEmpty(cmd.BootsectorFile))
            {
                byte[] bootSector = File.ReadAllBytes(cmd.BootsectorFile);
                int padSize = SectorSize - bootSector.Length % SectorSize;
                int reservedSectorCount = NumSectors(bootSector.Length);
                diskDesc.NumReservedSectors = reservedSectorCount;

                if (padSize != 0)
                {
                    Console.WriteLine($"Warning : boot sector is not a multiple of {SectorSize} bytes - padding with zero bytes to {reservedSectorCount} sector(s) ({reservedSectorCount * SectorSize} bytes).");
                }

                int bpbReservedSectorCount = BitConverter.ToUInt16(bootSector, 14);
                if (bpbReservedSectorCount != diskDesc.NumReservedSectors)
                {
                    Console.WriteLine($"Warning : the BIOS parameter block's reserved sector count (at offset 14) does not match the boot sector count. Expected {reservedSectorCount}, but found {bpbReservedSectorCount}.");
                }

                image.Write(bootSector, 0, bootSector.Length);
            }

            // write out the endianness and last cluster indicators
            SetFatEntry(diskDesc.Fat0Offset, 0, 0xFF0);
            SetFatEntry(diskDesc.Fat0Offset, 1, 0xFFF);
            SetFatEntry(diskDesc.Fat1Offset, 0, 0xFF0);
            SetFatEntry(diskDesc.Fat1Offset, 1, 0xFFF);

            foreach (var file in sourceFiles)
            {
                AddFileToImage(file);
            }

            File.WriteAllBytes(cmd.OutFile, image.ToArray());
        }


        static void AddFileToImage(SourceInfo file)
        {
            int fileSectors = NumSectors(file.Info.Length);

            byte[] fileBytes = File.ReadAllBytes(file.Info.FullName);

            SetRootEntry(
                diskDesc.NextUnusedRootEntry,
                diskDesc.NextUnusedCluster,
                file.Info,
                FatFileAttributes.System | FatFileAttributes.ReadOnly
            );

            diskDesc.NextUnusedRootEntry++;

            for (int i = 0; i < fileSectors; i++)
            {
                int thisCluster = diskDesc.NextUnusedCluster;

                int nextCluster = (i < fileSectors - 1)
                    ? thisCluster + 1
                    : 0xFFF;

                SetFatEntry(diskDesc.Fat0Offset, thisCluster, nextCluster);
                SetFatEntry(diskDesc.Fat1Offset, thisCluster, nextCluster);

                int startOffset = i * SectorSize;
                int sectorFillSize = Math.Min(SectorSize, (fileBytes.Length - startOffset));

                image.Position = diskDesc.NextUnusedSector * SectorSize;
                image.Write(fileBytes, startOffset, sectorFillSize);

                diskDesc.NextUnusedCluster++;
            }
        }


        static void SetFatEntry(long tableBaseOffset, int index, int value)
        {
            long pairOffset = tableBaseOffset + (index * 3) / 2;
            byte[] pair = new byte[2];

            image.Seek(pairOffset, SeekOrigin.Begin);
            image.Read(pair, 0, pair.Length);

            // clean up any extra bits
            value = (value & 0xFFF);

            if ((index & 1) == 0)
            {
                pair[0] = (byte)(value & 0xFF);
                pair[1] = (byte)((pair[1] & 0xF0) | (value >> 8));
            }
            else
            {
                pair[0] = (byte)((pair[0] & 0x0F) | ((value & 0x0F) << 4));
                pair[1] = (byte)(value >> 4);
            }

            image.Seek(pairOffset, SeekOrigin.Begin);
            image.Write(pair, 0, pair.Length);
        }

        static void SetRootEntry(int entryNumber, int startCluster, FileInfo origFile, FatFileAttributes attrs)
        {
            image.Position = diskDesc.RootDirOffset + 32 * entryNumber;

            // write 8-byte name
            string name = Path.GetFileNameWithoutExtension(origFile.Name);

            var bytes = new byte[8];
            bytes.Fill((byte)' ');
            Encoding.ASCII.GetBytes(name, 0, Math.Min(8, name.Length), bytes, 0);
            image.Write(bytes, 0, 8);

            // write 3-byte extension
            string ext = Path.GetExtension(origFile.Name);

            if (string.IsNullOrEmpty(ext))
            {
                ext = string.Empty;
            }
            else if (ext.Length > 0)
            {
                ext = ext.Substring(1, 3);
            }

            bytes.Fill((byte)' ');
            Encoding.ASCII.GetBytes(ext, 0, Math.Min(3, ext.Length), bytes, 0);
            image.Write(bytes, 0, 3);

            // attributes
            image.WriteByte((byte)attrs);

            // WinNT reserved
            image.WriteByte(0);

            // creation time (10ths of a second):
            image.WriteByte((byte)(((origFile.CreationTime.Second & 1) == 0) ? 10 : 0));

            // creation time rest:
            bytes.Fill((byte)0);
            FillDateTime(bytes, origFile.CreationTime);
            image.Write(bytes, 0, 4);

            // skip access date and high word of start cluster number
            image.Position += 4;

            // modification time:
            FillDateTime(bytes, origFile.LastWriteTime);
            image.Write(bytes, 0, 4);

            // low word of first cluster number:
            bytes = BitConverter.GetBytes((ushort)(startCluster & 0xFFFF));
            image.Write(bytes, 0, sizeof(ushort));

            // file size:
            bytes = BitConverter.GetBytes((uint)origFile.Length);
            image.Write(bytes, 0, sizeof(uint));
        }


        static int NumSectors(long byteSize)
        {
            if (byteSize > int.MaxValue)
                throw new ArgumentOutOfRangeException(nameof(byteSize));

            return NumSectors((int)byteSize);
        }

        static int NumSectors(int byteSize)
        {
            // = ceil (byteSize / SectorSize)
            return (byteSize + SectorSize - 1) / SectorSize;
        }

        static void FillDateTime(byte[] bytes, DateTime time)
        {
            Debug.Assert(bytes != null);
            Debug.Assert(bytes.Length >= 4);

            int hour = time.Hour & 0x1F;
            int min = time.Minute & 0x3F;
            int sec = (time.Second >> 1) & 0x1F;

            ushort word = (ushort)((hour << 11) | (min << 5) | (sec));
            Array.Copy(BitConverter.GetBytes(word), bytes, 0);

            int year = (time.Year - 1980) & 0x7F;
            int month = time.Month & 0x0F;
            int day = time.Day & 0x1F;

            word = (ushort)((year << 9) | (month << 5) | day);
            Array.Copy(BitConverter.GetBytes(word), bytes, 2);
        }


        class SourceInfo
        {
            public FileInfo Info { get; }

            public SourceInfo(string filePath)
                : this(new FileInfo(filePath))
            { }

            public SourceInfo(FileInfo fileInfo)
            {
                Info = fileInfo;
            }
        }
    }
}
