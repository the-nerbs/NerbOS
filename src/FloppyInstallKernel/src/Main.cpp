#include "kstdint.h"
#include "sal.h"
#include "intrin.h"
#include "physmem.h"
#include "vgatext.h"

NOS_EXTERN_C

static bool RunGlobalCtors();

// 0x000110a0
void kmain(_In_ MemoryMap* mmap)
{
    if (!RunGlobalCtors())
    {
        __bochsbreak();
    }

    vtInitialize();
    vtClearScreen();
    vtSetCursorPos(0, 0);
    vtEnableCursor();

    vtPrint("In kmain\n");
    vtPrint("Initializing memory . . .\n");

    pmInitialize(mmap);

    vtPrint("\nHit end of kmain...");
    __bochsbreak();
}

// MSVC initialization sections:
#pragma section(".CRT$XIA",    long, read) // First C Initializer
#pragma section(".CRT$XIZ",    long, read) // Last C Initializer
#pragma section(".CRT$XCA",    long, read) // First C++ Initializer
#pragma section(".CRT$XCZ",    long, read) // Last C++ Initializer

using CInitializerFn = int(__cdecl*)();
using CppInitializerFn = void(__cdecl*)();

__declspec(allocate(".CRT$XIA")) CInitializerFn __xi_a[] = { nullptr };     // C initializers (first)
__declspec(allocate(".CRT$XIZ")) CInitializerFn __xi_z[] = { nullptr };     // C initializers (last)
__declspec(allocate(".CRT$XCA")) CppInitializerFn __xc_a[] = { nullptr };   // C++ initializers (first)
__declspec(allocate(".CRT$XCZ")) CppInitializerFn __xc_z[] = { nullptr };   // C++ initializers (last)


bool RunGlobalCtors()
{
    int result = 0;

    // run C initializers
    for (CInitializerFn* init = __xi_a;
        init < __xi_z;
        init++)
    {
        if (*init != nullptr)
        {
            result = (**init)();
            if (result != 0)
            {
                __bochsbreak();
                break;
            }
        }
    }

    if (result == 0)
    {
        // run C++ initializers
        for (CppInitializerFn* init = __xc_a;
            init < __xc_z;
            init++)
        {
            if (*init != nullptr)
            {
                (**init)();
            }
        }
    }

    return (result == 0);
}

NOS_END_EXTERN_C
