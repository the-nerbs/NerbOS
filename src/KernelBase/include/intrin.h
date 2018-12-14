//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Declares wrapper functions for invoking individual opcodes.
//!
//! \note   These aren't truly intrinsics, as there is no compiler support. Using these
//!         extensively will likely cause performance issues.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "kstdint.h"
#include "kstddef.h"
#include "platform.h"
#include "compilerintrin.h"

NOS_EXTERN_C

#define INTRIN(func)            func
#define INTRIN_X86(func)        
#define INTRIN_X64(func)        
#define INTRIN_X86_X64(func)    

#if NOS_PLATFORM == NOS_PLATFORM_x86
#undef  INTRIN_X86
#define INTRIN_X86(func)        INTRIN(func)
#endif

#if NOS_PLATFORM == NOS_PLATFORM_x64
#undef  INTRIN_X64
#define INTRIN_X64(func)        INTRIN(func)
#endif

#if NOS_PLATFORM == NOS_PLATFORM_x86 || NOS_PLATFORM == NOS_PLATFORM_x64
#undef  INTRIN_X86_X64
#define INTRIN_X86_X64(func)    INTRIN(func)

// x86 or x64 specific - define some helpers for interpreting the cpuid results.
typedef struct tag_cpuid_result
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result;

inline cpuid_result cpuid(int function)
{
    int data[4];
    __cpuid(data, function);
    return *(cpuid_result*)data;
}

inline cpuid_result cpuidex(int function, int subfunction)
{
    int data[4];
    __cpuidex(data, function, subfunction);
    return *(cpuid_result*)data;
}

#endif


INTRIN_X86_X64(void __bochsbreak());

// standard library functions which undergo intrinsic replacement by the compiler.
// see: https://docs.microsoft.com/en-us/cpp/preprocessor/intrinsic?view=vs-2017
INTRIN(unsigned long _lrotl(unsigned long value, int shift));
INTRIN(unsigned long _lrotr(unsigned long value, int shift));
INTRIN(unsigned int _rotl(unsigned int value, int shift));
INTRIN(unsigned int _rotr(unsigned int value, int shift));
INTRIN(int abs(int x));
INTRIN(long labs(long x));
INTRIN(int memcmp(const void* buffer1, const void* buffer2, size_t count));
INTRIN(void* memcpy(void* dest, const void* src, size_t count));
INTRIN(void* memset(void* dest, int ch, size_t count));
INTRIN(size_t strlen(const char* psz));

// these aren't listed on the MSDocs page, but they have assembly code next to memcmp/memcpy/memset...
// TODO: are these well-known to the compiler like the others?
// INTRIN(void* memchr(const void* ptr, int ch, size_t count));
// INTRIN(void* memmove(void* dest, const void* src, size_t count));


#undef INTRIN_X86_X64
#undef INTRIN_X64
#undef INTRIN_X86
#undef INTRIN

NOS_END_EXTERN_C
