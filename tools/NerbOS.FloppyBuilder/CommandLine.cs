using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NerbOS.FloppyBuilder
{
    class CommandLine
    {
        public string BootsectorFile { get; private set; }
        public string SourceDir { get; private set; }
        public string OutFile { get; private set; }
        public List<string> SourceFiles { get; private set; }
            = new List<string>();


        public static bool TryParse(string[] args, out CommandLine parsed)
        {
            var cmd = new CommandLine();

            foreach (var item in args)
            {
                if (TryParseArg(item, out string name, out string value))
                {
                    if (MatchesAny(name, "boot", "b"))
                    {
                        if (!string.IsNullOrEmpty(cmd.BootsectorFile))
                        {
                            goto failure;
                        }

                        cmd.BootsectorFile = value;
                    }
                    else if (MatchesAny(name, "in", "i"))
                    {
                        if (!string.IsNullOrEmpty(cmd.SourceDir))
                        {
                            goto failure;
                        }

                        cmd.SourceDir = value;
                    }
                    else if (MatchesAny(name, "out", "o"))
                    {
                        if (!string.IsNullOrEmpty(cmd.OutFile))
                        {
                            goto failure;
                        }

                        cmd.OutFile = value;
                    }
                    else
                    {
                        goto failure;
                    }
                }
                else
                {
                    cmd.SourceFiles.Add(item);
                }
            }

            parsed = cmd;
            return true;

        failure:
            parsed = null;
            return false;
        }


        private CommandLine()
        { }


        private static bool IsArg(string str)
        {
            return str.Length > 0
                && (str[0] == '-' || str[0] == '/');
        }

        private static bool TryParseArg(string arg, out string name, out string value)
        {
            if (IsArg(arg))
            {
                int colonIdx = arg.IndexOf(':');

                if (colonIdx < 0)
                {
                    name = arg.Substring(1);
                    value = null;
                }
                else
                {
                    name = arg.Substring(1, colonIdx - 1);
                    value = arg.Substring(colonIdx + 1);
                }

                return true;
            }

            name = null;
            value = null;
            return false;
        }

        private static bool MatchesAny(string name, params string[] alternatives)
        {
            return alternatives.Any(a => StringComparer.CurrentCultureIgnoreCase.Equals(a, name));
        }
    }
}
