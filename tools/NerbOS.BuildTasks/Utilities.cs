using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NerbOS.BuildTasks
{
    internal static class Utilities
    {
        public static string FullToolPath(string pathValue, string toolExeName)
        {
            string toolPath;

            if (!string.IsNullOrWhiteSpace(pathValue))
            {
                if (Path.HasExtension(pathValue))
                {
                    // we have an extension, treat it as the full executable path.
                    toolPath = pathValue;
                }
                else
                {
                    // no extension, treat it like it's just the directory
                    toolPath = Path.Combine(pathValue, toolExeName);
                }

                toolPath = Path.GetFullPath(toolPath);
            }
            else
            {
                // no path specified, assume the tool is on PATH.
                toolPath = toolExeName;
            }

            return toolPath;
        }
    }
}
