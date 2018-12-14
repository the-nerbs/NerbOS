#include "physmem.h"
#include "platform.h"
#include "kstddef.h"

NOS_EXTERN_C

static uint64_t pageSize;
static uint64_t totalMemory;
static uint64_t allocatedMemory;


_Use_decl_annotations_
void pmInitialize(MemoryMap* mmap)
{
    // TODO: setup the book-keeping structures and populate with the memory map.
    NOS_UNUSED_PARAM(mmap);
//    size_t totalSize = offsetof(MemoryMap, entries)
//                     + mmap->count * sizeof(MemMapEntry);
}

uint64_t pmPageSize()
{
    return pageSize;
}

uint64_t pmTotalMemory()
{
    return totalMemory;
}

uint64_t pmAllocatedMemory()
{
    return allocatedMemory;
}

_Use_decl_annotations_
void* pmAllocateBytes(uint32_t cb, void* hint, uint32_t* pageCount)
{
    // TODO: allocate
    NOS_UNUSED_PARAM(cb);
    NOS_UNUSED_PARAM(hint);

    *pageCount = 0;
    return nullptr;
}

_Use_decl_annotations_
void* pmAllocatePages(uint32_t pageCount, void* hint)
{
    // TODO: allocate
    NOS_UNUSED_PARAM(pageCount);
    NOS_UNUSED_PARAM(hint);

    return nullptr;
}

_Use_decl_annotations_
void pnFree(void* ptr, uint32_t pageCount)
{
    // TODO: free
    NOS_UNUSED_PARAM(ptr);
    NOS_UNUSED_PARAM(pageCount);
}

NOS_END_EXTERN_C
