using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NerbOS.FloppyBuilder
{
    internal static class ExtensionMethods
    {
        public static void Fill<T>(this T[] array, T value)
        {
            for (int i = 0, n = array.Length; i < n; i++)
            {
                array[i] = value;
            }
        }
    }
}
