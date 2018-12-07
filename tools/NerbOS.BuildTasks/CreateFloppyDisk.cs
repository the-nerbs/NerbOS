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

        public ITaskItem[] BootstrapFile { get; set; }

        [Required]
        public string ToolsDirectory { get; set; }

        [Required]
        public string OutputPath { get; set; }

        [Output]
        public ITaskItem[] Outputs { get; set; }

        protected override string ToolName
        {
            get { return "FloppyBuilder.exe"; }
        }


        protected override string GenerateFullPathToTool()
        {
            return Utilities.FullToolPath(ToolsDirectory, ToolName);
        }

        protected override string GenerateCommandLineCommands()
        {
            var cmdLine = new CommandLineBuilder();
            var outputs = new List<ITaskItem>();

            ITaskItem bootstrap = null;
            if (BootstrapFile != null)
            {
                if (BootstrapFile.Length > 1)
                {
                    Log.LogError("Multiple bootstrap files is not supported.");
                    return null;
                }

                bootstrap = BootstrapFile[0];
            }

            cmdLine.AppendSwitchIfNotNull("/b:", bootstrap);
            cmdLine.AppendSwitchIfNotNull("/o:", OutputPath);
            cmdLine.AppendFileNamesIfNotNull(Sources, " ");

            Outputs = outputs.ToArray();
            return cmdLine.ToString();
        }
    }
}
