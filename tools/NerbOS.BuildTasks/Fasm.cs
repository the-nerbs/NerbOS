using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace NerbOS.BuildTasks
{
    public class Fasm : ToolTask
    {
        [Required]
        public ITaskItem Source { get; set; }

        [Required]
        public string FasmPath { get; set; }

        [Required]
        public string OutputDirectory { get; set; }

        public bool GenerateSymbols { get; set; }

        [Output]
        public ITaskItem[] Outputs { get; set; }

        protected override string ToolName
        {
            get { return "FASM.exe"; }
        }


        protected override string GenerateFullPathToTool()
        {
            return Utilities.FullToolPath(FasmPath, ToolName);
        }

        protected override string GenerateCommandLineCommands()
        {
            var cmdLine = new CommandLineBuilder();
            var outputItems = new List<ITaskItem>();

            string outputName = Path.GetFileNameWithoutExtension(Source.ItemSpec);
            string outputFile = Path.Combine(OutputDirectory, Path.ChangeExtension(outputName, ".bin"));

            cmdLine.AppendFileNameIfNotNull(Source.GetMetadata("FullPath"));

            outputItems.Add(new TaskItem(outputFile));
            cmdLine.AppendFileNameIfNotNull(outputFile);

            if (GenerateSymbols)
            {
                string symbolFile = Path.Combine(OutputDirectory, Path.ChangeExtension(outputName, ".fas"));
                cmdLine.AppendSwitch("-s");
                cmdLine.AppendFileNameIfNotNull(symbolFile);

                outputItems.Add(new TaskItem(symbolFile));
            }

            Outputs = outputItems.ToArray();
            return cmdLine.ToString();
        }
    }
}
