using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace NerbOS.BuildTasks
{
    public class CreateFloppyDisk : ToolTask
    {
        [Required]
        public ITaskItem[] Sources { get; set; }

        [Required]
        public string OutputPath { get; set; }

        public string BootSectorFile { get; set; }

        [Output]
        public ITaskItem[] Outputs { get; set; }

        protected override string ToolName
        {
            get { return "FloppyBuilder.exe"; }
        }


        protected override string GenerateFullPathToTool()
        {
            return Utilities.FullToolPath(ToolPath, ToolName);
        }

        protected override string GenerateCommandLineCommands()
        {
            var cmdLine = new CommandLineBuilder();
            var outputs = new List<ITaskItem>();

            cmdLine.AppendSwitchIfNotNull("/b:", BootSectorFile);
            cmdLine.AppendSwitchIfNotNull("/o:", OutputPath);
            cmdLine.AppendFileNamesIfNotNull(Sources, " ");

            Outputs = outputs.ToArray();
            return cmdLine.ToString();
        }
    }
}
