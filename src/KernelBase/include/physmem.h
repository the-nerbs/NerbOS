//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Defines the interface for physical memory management.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "nosbase.h"
#include "kstdint.h"
#include "sal.h"

NOS_EXTERN_C

//-----------------------------------------------------------------------------
//! \brief  Indicates the type of memory described by a MemMapEntry.
//! Defined by BIOS INT 0x15, EAX = 0xE820
//-----------------------------------------------------------------------------
enum MemMapRegionType
{
    MMRT_Usable          = 1,   //!< Memory region is usable by the OS.
    MMRT_Reserved        = 2,   //!< Memory region is reserved.
    MMRT_AcpiReclaimable = 3,   //!< Memory region is initialized with ACPI
                                //!<    tables, but is usable by the OS.
    MMRT_AcpiNvsMemory   = 4,   //!< ACPI non-volatile memory region.
    MMRT_BadMemory       = 5,   //!< Memory region is not usable.
};

//-----------------------------------------------------------------------------
//! \brief  Memory region descriptor.
//! Defined by BIOS INT 0x15, EAX = 0xE820
//-----------------------------------------------------------------------------
struct MemMapEntry
{
    uint64_t base;              //!< Base address of the memory region.
    uint64_t length;            //!< Length of the memory region.
    uint32_t regionType;        //!< Type of the memory region.
    uint32_t acpiExtAttributes; //!< ACPI extended attributes.
};
static_assert(sizeof(MemMapEntry) == 24, "Unexpected size of MemMapEntry");

//-----------------------------------------------------------------------------
//! \brief  Memory descriptor.
//! Defined by BIOS INT 0x15, EAX = 0xE820
//-----------------------------------------------------------------------------
struct MemoryMap
{
    int32_t count;              //!< number of entries.
    uint32_t padding;           //!< unused padding.

    _Field_size_(count)
    MemMapEntry entries[1];     //!< variable sized array of memory map entries.
};


//-----------------------------------------------------------------------------
//! \brief  Initialized the physical memory manager.
//!
//! \param  mmap  The physical memory map.
//-----------------------------------------------------------------------------
void pmInitialize(_In_ MemoryMap* mmap);

//-----------------------------------------------------------------------------
//! \brief  Gets the size of a physical memory page.
//-----------------------------------------------------------------------------
uint64_t pmPageSize();

//-----------------------------------------------------------------------------
//! \brief  Gets the total amount of usable memory.
//-----------------------------------------------------------------------------
uint64_t pmTotalMemory();

//-----------------------------------------------------------------------------
//! \brief  Gets the total amount of allocated memory.
//-----------------------------------------------------------------------------
uint64_t pmAllocatedMemory();

//-----------------------------------------------------------------------------
//! \brief  Allocates enough pages of contiguous physical memory to fit the
//!         requested number of bytes.
//!
//! \param       cb         The minimum number of bytes allocated.
//! \param       hint       If not null, a position at which to try to allocate
//!                         the requested memory.
//! \param[out]  pageCount  The number of pages allocated.
//!
//! \returns  A pointer to the allocated pages, or null if there is not enough
//!           contiguous memory to satisfy the request.
//-----------------------------------------------------------------------------
void* pmAllocateBytes(uint32_t cb, _In_opt_ void* hint, _Out_ uint32_t* pageCount);

//-----------------------------------------------------------------------------
//! \brief  Allocates the given number of pages of contiguous physical memory.
//! 
//! \param  pageCount  The number of pages to allocate.
//! \param  hint       If not null, a position at which to try to allocate the
//!                    requested memory.
//!
//! \returns  A pointer to the allocated pages, or null if there is not enough
//!           contiguous memory to satisfy the request.
//-----------------------------------------------------------------------------
void* pmAllocatePages(uint32_t pageCount, _In_opt_ void* hint);

//-----------------------------------------------------------------------------
//! \brief  Frees the given number of pages at the given point in memory.
//!
//! \param  ptr        A pointer to the base of the memory to free.
//! \param  pageCount  The number of pages to free.
//-----------------------------------------------------------------------------
void pnFree(_In_ void* ptr, uint32_t pageCount);

NOS_END_EXTERN_C
