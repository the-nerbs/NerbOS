#include "physmem.h"
#include "platform.h"

NOS_EXTERN_C

static uint64_t pageSize;
static uint64_t totalMemory;
static uint64_t allocatedMemory;


_Use_decl_annotations_
void pmInitialize(MemoryMap* mmap)
{
    // TODO: setup the book-keeping structures and populate with the memory map.
    ((void)mmap);
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
    ((void)cb);
    ((void)hint);

    *pageCount = 0;
    return nullptr;
}

_Use_decl_annotations_
void* pmAllocatePages(uint32_t pageCount, void* hint)
{
    // TODO: allocate
    ((void)pageCount);
    ((void)hint);

    return nullptr;
}

_Use_decl_annotations_
void pnFree(void* ptr, uint32_t pageCount)
{
    // TODO: free
    ((void)ptr);
    ((void)pageCount);
}

NOS_END_EXTERN_C
