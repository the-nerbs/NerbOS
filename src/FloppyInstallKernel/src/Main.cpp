#include "kstdint.h"
#include "krtinit.h"
#include "sal.h"
#include "intrin.h"
#include "physmem.h"
#include "vgatext.h"

NOS_EXTERN_C

// 0x000110a0
void kmain(_In_ MemoryMap* mmap)
{
    if (!nos_krt_init())
    {
        __bochsbreak();
    }

    vtInitialize();
    vtClearScreen();
    vtSetCursorPos(0, 0);
    vtEnableCursor();

    vtPrintString("In kmain\n");

    __bochsbreak();
    kprintf(vtKPrintfStream(), "Memory Map (%d entries):\n", mmap->count);
    __bochsbreak();

    for (int i = 0; i < mmap->count; i++)
    {
        kprintf(
            vtKPrintfStream(),
            "%016llX %016llX %016llX %d %08X\n",
            mmap->entries[i].base,
            mmap->entries[i].length,
            (mmap->entries[i].base + mmap->entries[i].length),
            mmap->entries[i].regionType,
            mmap->entries[i].acpiExtAttributes
        );
    }

    vtPrintString("\nInitializing memory . . .\n");

    pmInitialize(mmap);

    vtPrintString("Hit end of kmain . . .\n");
    __bochsbreak();
}


NOS_END_EXTERN_C
