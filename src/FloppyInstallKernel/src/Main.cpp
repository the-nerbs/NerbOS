#include "kstdint.h"
#include "krtinit.h"
#include "sal.h"
#include "intrin.h"
#include "physmem.h"
#include "vgatext.h"

NOS_EXTERN_C

static void mask_all_irqs()
{
    enum // just some local constants
    {
        PIC1 = 0x20,         // IO base address for master PIC
        PIC2 = 0xA0,         // IO base address for slave PIC

        PIC1_COMMAND = PIC1,
        PIC1_DATA = (PIC1 + 1),

        PIC2_COMMAND = PIC2,
        PIC2_DATA = (PIC2 + 1),
    };

    __outbyte(PIC1_DATA, 0xFF);
    __outbyte(PIC2_DATA, 0xFF);
}

void kmain(_In_ MemoryMap* mmap)
{
    // mask all IRQs - since I don't have anything to remap IRQs 0-7 to other
    // interrupts yet, these would raise interrupts that map to exception codes,
    // and ultimately lead to one very confused developer and about a week of
    // debugging spurious #DF exceptions with no exceptions leading up to it.
    mask_all_irqs();

    if (!nos_krt_init())
    {
        __bochsbreak();
    }

    vtInitialize();
    vtClearScreen();
    vtSetCursorPos(0, 0);
    vtEnableCursor();

    vtPrintString("In kmain\n");
    kprintf(vtKPrintfStream(), "Memory Map (%d entries):\n", mmap->count);

    for (int i = 0; i < mmap->count; i++)
    {
        kprintf(
            vtKPrintfStream(),
            "%016llX - %016llX (%016llX) %d %08X\n",
            mmap->entries[i].base,
            (mmap->entries[i].base + mmap->entries[i].length),
            mmap->entries[i].length,
            mmap->entries[i].regionType,
            mmap->entries[i].acpiExtAttributes
        );
    }


    vtPrintString("Initializing memory . . .\n");

    if (!pmInitialize(mmap))
    {
        vtPrintString("Failed to initialize memory.\n\n");
    }

    vtPrintString("Hit end of kmain . . .\n");
    __bochsbreak();
}

NOS_END_EXTERN_C
