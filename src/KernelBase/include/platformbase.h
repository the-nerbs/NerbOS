//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Defines the available values for platform-specific configurations
//!
//! \details
//! Each platform defines a header named `platform.h` which describes the parameters of the
//! target platform.  The macros that must be defined on all platforms are:
//!
//! |   Macro               | Description                                                       |
//! |-----------------------|-------------------------------------------------------------------|
//! | `NOS_PLATFORM`        | Provides a value uniquely indicating the target architecture.     |
//! | `NOS_PTR_SIZE`        | Provides the size of pointers (in bytes) of the target platform.  |
//! | `NOS_PTR_USABLE_BITS` | Provides the number of bits that can be used in a pointer.        |
//! | `NOS_PTR_MASK`        | Provides a mask of the usable bits in a pointer.                  |
//-------------------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
//  NOS_PLATFORM
//-----------------------------------------------------------------------------
#define NOS_PLATFORM_x86    86      //!< x86 (IA32)
#define NOS_PLATFORM_x64    8664    //!< x64 (AMD64, x86_64)


//-----------------------------------------------------------------------------
// NOS_PTR_SIZE
//-----------------------------------------------------------------------------
#define NOS_PTR_SIZE_32BIT  4       //!< 32-bit pointers
#define NOS_PTR_SIZE_64BIT  8       //!< 64-bit pointers

//-----------------------------------------------------------------------------
// NOS_PTR_MASK
//-----------------------------------------------------------------------------
#define NOS_PTR_MASK    (((1ULL << (NOS_PTR_USABLE_BITS-1)) << 1) | 1)
