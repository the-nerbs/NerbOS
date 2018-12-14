//-------------------------------------------------------------------------------------------------
//! \file
//-------------------------------------------------------------------------------------------------
#pragma once
#include "nosbase.h"
#include "kstdargs.h"
#include "sal.h"

NOS_EXTERN_C

typedef struct tag_kprintf_stream
{
    void (*write)(char ch);
} kprintf_stream;


void kprintf(
    _In_ const kprintf_stream* stream,
    _In_ _Printf_format_string_ const char* fmt,
    ...
);

void kvprintf(
    _In_ const kprintf_stream* stream,
    _In_ _Printf_format_string_ const char* fmt,
    va_list args
);

NOS_END_EXTERN_C
