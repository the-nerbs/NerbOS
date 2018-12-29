#include "krtinit.h"
#include "kstddef.h"
#include "intrin.h"

typedef _Bool   bool;
#define true    1
#define false   0

// MSVC initialization sections:
// see: https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization?view=vs-2017
#pragma section(".CRT$XIA",    long, read) // First C Initializer
#pragma section(".CRT$XIZ",    long, read) // Last C Initializer
#pragma section(".CRT$XCA",    long, read) // First C++ Initializer
#pragma section(".CRT$XCZ",    long, read) // Last C++ Initializer

typedef int (__cdecl *CInitializerFn)();
typedef void (__cdecl *CppInitializerFn)();

__declspec(allocate(".CRT$XIA")) CInitializerFn __xi_a[] = { NULL };     // C initializers (first)
__declspec(allocate(".CRT$XIZ")) CInitializerFn __xi_z[] = { NULL };     // C initializers (last)
__declspec(allocate(".CRT$XCA")) CppInitializerFn __xc_a[] = { NULL };   // C++ initializers (first)
__declspec(allocate(".CRT$XCZ")) CppInitializerFn __xc_z[] = { NULL };   // C++ initializers (last)

// MSVC isa availability/favoring
enum // constants
{
    isa_X86 = 0,
    isa_SSE2 = 1,
    isa_SSE42 = 2,
    isa_AVX = 3,
    isa_ERMSB = 4,
    isa_AVX2 = 5,

    favor_ATOM = 0,
    favor_ERMSB = 1,
    favor_SmallStrings = 2, // x64 only
};


uint32_t __isa_available;
uint32_t __isa_enabled;
uint32_t __favor;


static bool run_global_ctors()
{
    int result = 0;

    // run C initializers
    for (CInitializerFn* init = __xi_a;
        init < __xi_z && result == 0;
        init++)
    {
        if (*init != NULL)
        {
            result = (**init)();
#ifndef NDBEUG
            if (result != 0)
            {
                __debugbreak();
            }
#endif
        }
    }

    if (result == 0)
    {
        // run C++ initializers
        for (CppInitializerFn* init = __xc_a;
            init < __xc_z;
            init++)
        {
            if (*init != NULL)
            {
                (**init)();
            }
        }
    }

    return (result == 0);
}

static void init_isa_descriptors()
{
    cpuid_result result;

    __isa_available = isa_X86;
    __isa_enabled = (1 << isa_X86);
    __favor = favor_ATOM;

#ifdef NOS_FLOAT_SUPPORT
    result = cpuid(0x01);
    // ecx bit
    //  28 = AVX
    //  20 = SSE4.2
    //  19 = SSE4.1
    //   9 = SSSE3
    //   0 = SSE3
    //
    // edx bit
    //  26 = SSE2
    //  25 = SSE

    // SSE2 test
    if ((result.edx & 0x04000000) != 0)
    {
        __isa_available = isa_SSE2;
        __isa_enabled |= (1 << isa_SSE2);
    }

    // SSE4.2 test
    if ((result.ecx & 0x00100000) != 0)
    {
        __isa_available = isa_SSE42;
        __isa_enabled |= (1 << isa_SSE42);
    }

    // AVX test
    if ((result.ecx & 0x10000000) != 0)
    {
        __isa_available = isa_AVX;
        __isa_enabled |= (1 << isa_AVX);
    }

#endif // NOS_FLOAT_SUPPORT

    result = cpuid(0x07);
    // ebx bit
    //  9 = enhanced REP MOVSB/STOSB (ERMSB)
    //  5 = AVX2

    // ERMSB test
    if ((result.ebx & 0x00000200) != 0)
    {
        // note - it doesn't look like MSVCRT ever sets this level/bit...
        __isa_available = isa_ERMSB;
        __isa_enabled |= (1 << isa_ERMSB);
        __favor |= favor_ERMSB;
    }

#ifdef NOS_FLOAT_SUPPORT
    // AVX2 test
    if ((result.ebx & 0x00000020) != 0)
    {
        __isa_available = isa_AVX2;
        __isa_enabled |= (1 << isa_AVX2);
    }
#endif // NOS_FLOAT_SUPPORT
}

int nos_krt_init()
{
    if (run_global_ctors())
    {
        init_isa_descriptors();
        return true;
    }

    return false;
}
