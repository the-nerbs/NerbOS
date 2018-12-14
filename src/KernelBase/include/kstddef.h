//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Kernel-mode stddef.h
//-------------------------------------------------------------------------------------------------
#pragma once
#include "nosbase.h"
#include "platform.h"
#include "kstdint.h"

NOS_EXTERN_C

// size_t, ptrdiff_t defined in kstdint.h

#define NULL            ((void*)0)

#define offsetof(s, m)  ((size_t)(((s*)0)->m))

#define MIN(x,y)        (((x) < (y)) ? (x) : (y))
#define MAX(x,y)        (((x) > (y)) ? (x) : (y))

NOS_END_EXTERN_C
