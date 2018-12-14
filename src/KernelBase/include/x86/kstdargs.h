//-------------------------------------------------------------------------------------------------
//! \file
//-------------------------------------------------------------------------------------------------
#pragma once
#include "kstdint.h"

typedef char* va_list;

#ifdef  __cplusplus
#define _ADDRESSOF(x)   (&const_cast<char&>((reinterpret_cast<const volatile char&>(x))))
#else
#define _ADDRESSOF(x)   (&(x))
#endif

#define _INTSIZEOF(x)           ((sizeof(x) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#define va_start(ap, parmN)     ((void)(ap = (va_list)_ADDRESSOF(parmN) + _INTSIZEOF(parmN)))
#define va_arg(ap, type)        (*(type*)((ap += _INTSIZEOF(type)) - _INTSIZEOF(type)))
#define va_end(ap)              ((ap) = 0)
#define va_copy(dest, src)      ((dest) = (src))
