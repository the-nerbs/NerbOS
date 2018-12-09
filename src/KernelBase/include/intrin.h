//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Declares wrapper functions for invoking individual opcodes.
//!
//! \note   These aren't truly intrinsics, as there is no compiler support. Using these
//!         extensively will likely cause performance issues.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "kstdint.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INTRIN_X86(func)       
#define INTRIN_X64(func)       
#define INTRIN_X86_X64(func)   

#if NOS_PLATFORM == NOS_PLATFORM_x86
#undef  INTRIN_X86
#define INTRIN_X86(func)        func
#endif

#if NOS_PLATFORM == NOS_PLATFORM_x64
#undef  INTRIN_X64
#define INTRIN_X64(func)        func
#endif

#if NOS_PLATFORM == NOS_PLATFORM_x86 || NOS_PLATFORM == NOS_PLATFORM_x64
#undef  INTRIN_X86_X64
#define INTRIN_X86_X64(func)    func

typedef struct tag_cpuid_result
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result;

#endif

INTRIN_X86_X64(void __cpuid(uint32_t eax, cpuid_result* pResult));
INTRIN_X86_X64(void __cpuidex(uint32_t eax, uint32_t ecx, cpuid_result* pResult));

INTRIN_X86_X64(uint8_t __inb(uint16_t port));
INTRIN_X86_X64(uint16_t __inw(uint16_t port));
INTRIN_X86_X64(uint32_t __indw(uint16_t port));

INTRIN_X86_X64(void __outb(uint16_t port, uint8_t value));
INTRIN_X86_X64(void __outw(uint16_t port, uint16_t value));
INTRIN_X86_X64(void __outdw(uint16_t port, uint32_t value));

INTRIN_X86_X64(void __bochsbreak());

INTRIN_X86_X64(size_t strlen(const char* psz));
INTRIN_X86_X64(size_t wcslen(const wchar_t* psz));

#undef INTRIN

#ifdef __cplusplus
}   // extern "C"
#endif
