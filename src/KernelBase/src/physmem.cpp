//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Implementation of the physical memory interface.
//!
//TODO (multithreading or sooner?): Allocation/Deallocation lock
//-------------------------------------------------------------------------------------------------
#include "physmem.h"
#include "platform.h"
#include "kstddef.h"
#include "kstdint.h"
#include "intrin.h"

#include "vgatext.h"
#include "kprintf.h"

NOS_EXTERN_C

//-------------------------------------------------------------------------------------------------
// typedefs
//-------------------------------------------------------------------------------------------------
typedef uintptr_t bitmap_word_t;

//-------------------------------------------------------------------------------------------------
// constants
//-------------------------------------------------------------------------------------------------
enum // constants
{
    // FUTURE: consider smaller pages for low-memory systems
    // FUTURE: consider ability to allocate large pages?
    PageSize = 4096,    //!< The size of a single page of memory.

    BitsPerByte = 8,    //!< The number of bits in a single byte.

    BitsPerBitmapWord = BitsPerByte * sizeof(bitmap_word_t),
            //!< The number of bits in a bitmap_word_t.

    BitmapWordSize = PageSize * BitsPerBitmapWord,
            //!< The number of bytes tracked by a single bitmap_word_t.

    BitmapByteSize = PageSize * BitsPerByte,
            //!< The number of bytes tracked by a single byte of the bitmap.
};


//-------------------------------------------------------------------------------------------------
// data
//-------------------------------------------------------------------------------------------------
static uint64_t g_totalMemory;
static uint64_t g_allocatedMemory;
static bitmap_word_t* g_pageBitmap;

//-------------------------------------------------------------------------------------------------
// inline/static functions
//-------------------------------------------------------------------------------------------------
extern "C++"
{
    template <class T>
    inline T PageAlignDown(T addr)
    {
        return (addr & ~T{ PageSize - 1 });
    }

    template <class T>
    inline T PageAlignUp(T addr)
    {
        return ((addr + T{ PageSize - 1 }) & ~T{ PageSize - 1 });
    }
}

static void ConstructBitmap(uintptr_t address, size_t bitmapSize, _In_ const MemoryMap* mmap);

static void MarkUnused(uintptr_t pageAddress);
static void MarkUsed(uintptr_t pageAddress);

_Check_return_ _Success_(return != 0)
static uintptr_t FindUnused(
    uintptr_t start,
    uint64_t end,
    int numPages
);


//-------------------------------------------------------------------------------------------------
// interface implementation
//-------------------------------------------------------------------------------------------------
_Use_decl_annotations_
bool pmInitialize(const MemoryMap* mmap)
{
    // find the extent of known memory.
    g_totalMemory = 0;
    for (int i = 0; i < mmap->count; i++)
    {
        const MemMapEntry* entry = &(mmap->entries[i]);

        if (entry->regionType == MMRT_Usable
            || entry->regionType == MMRT_AcpiReclaimable)
        {
            uint64_t entryTop = entry->base + entry->length;

            if (entryTop > g_totalMemory)
            {
                g_totalMemory = entryTop;
            }
        }
    }

    // cap the total memory to the max pointer value.
    if (g_totalMemory > UINTPTR_MAX)
    {
        g_totalMemory = UINTPTR_MAX;
    }

    const ptrdiff_t numBitmapWords = static_cast<ptrdiff_t>((g_totalMemory + (BitmapWordSize-1)) / BitmapWordSize);
    const ptrdiff_t bitmapSize = numBitmapWords * sizeof(bitmap_word_t);
    uintptr_t bitmapAddr = 0;

    // find a place for the page bitmap
    for (int i = 0; i < mmap->count; i++)
    {
        const MemMapEntry* entry = &(mmap->entries[i]);
        if (entry->regionType == MMRT_Usable)
        {
            uint64_t entryTop = entry->base + entry->length;
            uintptr_t usableTop = (uintptr_t)MIN(entryTop, UINTPTR_MAX);

            // make sure the clamped top is inside the region
            if (usableTop > entry->base)
            {
                ptrdiff_t usableSize = (ptrdiff_t)(usableTop - entry->base);
                if (usableSize > bitmapSize)
                {
                    uintptr_t bitmapAddrTest = PageAlignDown(usableTop - bitmapSize);

                    // make sure the base is still inside the region.
                    if (bitmapAddrTest > entry->base)
                    {
                        // keep the bitmap at the highest memory hintAddress possible.
                        bitmapAddr = MAX(bitmapAddr, bitmapAddrTest);
                    }
                }
            }
        }
    }

    kprintf(vtKPrintfStream(), "    memory bitmap = %p (len = %08x)\n", (void*)bitmapAddr, bitmapSize);

    if (bitmapAddr > 0)
    {
        ConstructBitmap(bitmapAddr, bitmapSize, mmap);
        return true;
    }

    return false;
}

uint64_t pmPageSize()
{
    return PageSize;
}

uint64_t pmTotalMemory()
{
    return g_totalMemory;
}

uint64_t pmAllocatedMemory()
{
    return g_allocatedMemory;
}

_Use_decl_annotations_
void* pmAllocateBytes(uint32_t cb, void* hint, uint32_t* pageCount)
{
    //TODO: kassert(g_pageBitmap != nullptr);

    const uint32_t numPages = PageAlignUp(cb) / PageSize;
    *pageCount = numPages;

    return pmAllocatePages(numPages, hint);
}

_Use_decl_annotations_
void* pmAllocatePages(uint32_t pageCount, void* hint)
{
    //TODO: kassert(g_pageBitmap != nullptr);

    // if 0 pages was requested, then just return a null pointer.
    if (pageCount == 0)
    {
        return nullptr;
    }

    uintptr_t hintAddress = (uintptr_t)hint;
    if (hintAddress > g_totalMemory)
    {
        hintAddress = 0;
    }

    // search [hint, end) for an open spot
    uintptr_t foundAddress = FindUnused(hintAddress, g_totalMemory, pageCount);

    // search [start, hint) for an open spot if we need to
    if (foundAddress == 0
        && hintAddress != 0)
    {
        foundAddress = FindUnused(0, hintAddress, pageCount);
    }

    if (foundAddress != 0)
    {
        uintptr_t markAddr = foundAddress;

        for (uint32_t i = 0;
            i < pageCount;
            i++, markAddr += PageSize)
        {
            MarkUsed(markAddr);
        }

        g_allocatedMemory += uint64_t{ pageCount } * PageSize;
        return (void*)foundAddress;
    }

    // out of memory
    return nullptr;
}

_Use_decl_annotations_
void pmFree(void* ptr, uint32_t pageCount)
{
    uintptr_t baseAddr = (uintptr_t)ptr;

    for (uint32_t i = 0;
        i < pageCount && baseAddr >= (uintptr_t)ptr && baseAddr < g_totalMemory;
        i++, baseAddr += PageSize)
    {
        //TODO: kassert(IsUsed(baseAddr));
        MarkUnused(baseAddr);
    }

    g_allocatedMemory -= uint64_t{ pageCount } * PageSize;
}


//-------------------------------------------------------------------------------------------------
// static function implementations
//-------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void ConstructBitmap(uintptr_t address, size_t bitmapSize, const MemoryMap* mmap)
{
    g_pageBitmap = (bitmap_word_t*)address;

    // assume all memory is used unless there's a region specifically calling it out as usable.
    memset(g_pageBitmap, ~0, bitmapSize);

    // mark the usable regions as free
    for (int i = 0; i < mmap->count; i++)
    {
        const MemMapEntry* entry = &(mmap->entries[i]);

        if (entry->regionType == MMRT_Usable)
        {
            uintptr_t baseAddr = (uintptr_t)PageAlignUp(entry->base);
            const uintptr_t endAddr = (uintptr_t)PageAlignDown(entry->base + entry->length);

            while (baseAddr < endAddr)
            {
                MarkUnused(baseAddr);
                baseAddr += PageSize;
            }
        }
    }

    // mark the bitmap region itself as used
    {
        uintptr_t baseAddr = (uintptr_t)PageAlignDown(address);
        uintptr_t endAddr = (uintptr_t)PageAlignUp(address + bitmapSize);

        // if we overflowed, fill up to the top of memory.
        if (endAddr < baseAddr)
        {
            endAddr = (uintptr_t)g_totalMemory;
        }

        while (baseAddr < endAddr)
        {
            MarkUsed(baseAddr);
            baseAddr += PageSize;
        }
    }
}

void MarkUnused(uintptr_t pageAddress)
{
    uintptr_t pageNumber = pageAddress / PageSize;
    int pageWord = pageNumber / BitsPerBitmapWord;

    bitmap_word_t& bitmapWord = g_pageBitmap[pageWord];

    int pageBit = pageNumber % BitsPerBitmapWord;

    bitmapWord &= ~(1 << pageBit);
}

void MarkUsed(uintptr_t pageAddress)
{
    uintptr_t pageNumber = pageAddress / PageSize;
    int pageWord = pageNumber / BitsPerBitmapWord;

    bitmap_word_t& bitmapWord = g_pageBitmap[pageWord];

    int pageBit = pageNumber % BitsPerBitmapWord;
    bitmapWord |= (1 << pageBit);
}

_Use_decl_annotations_
uintptr_t FindUnused(uintptr_t start, uint64_t end, int numPages)
{
    uintptr_t wordAddress = start;
    uintptr_t baseAddress = 0;
    uintptr_t foundAddress = 0;

    bitmap_word_t* bitmap = (g_pageBitmap + (start / BitmapWordSize));
    bitmap_word_t *const bitmapEnd = (g_pageBitmap + (end / BitmapWordSize));

    int remainingPages = numPages;

    // search [start, end) for a section of numPages contiguous free pages.
    //FUTURE: there *must* be a better algorithm for this, but this'll work for now... 
    while (bitmap < bitmapEnd
        && foundAddress == 0)
    {
        if (*bitmap != ~uintptr_t{ 0 })
        {
            // block of pages has some free space.
            for (int bit = 0; bit < BitsPerBitmapWord; bit++)
            {
                if ((*bitmap & (1 << bit)) == 0)
                {
                    // free page
                    if (baseAddress == 0)
                    {
                        baseAddress = wordAddress + bit * PageSize;
                    }

                    remainingPages--;
                    if (remainingPages == 0)
                    {
                        foundAddress = baseAddress;
                    }
                }
                else
                {
                    // page is already taken
                    baseAddress = 0;
                    remainingPages = numPages;
                }
            }
        }
        else
        {
            // block of pages is already taken, move to the next.
            remainingPages = numPages;
            baseAddress = 0;
        }

        bitmap++;
        wordAddress += BitmapWordSize;
    }

    return foundAddress;
}

NOS_END_EXTERN_C
