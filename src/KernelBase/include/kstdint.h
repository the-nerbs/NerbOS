//-------------------------------------------------------------------------------------------------
//! \file
//! \brief Defines a set of signed and unsigned integer types.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "platform.h"

//-----------------------------------------------------------------------------
// Signed integers
//-----------------------------------------------------------------------------
typedef signed char         int8_t;                         //!< 8-bit signed integer
typedef short               int16_t;                        //!< 16-bit signed integer
typedef int                 int32_t;                        //!< 32-bit signed integer
typedef long long           int64_t;                        //!< 64-bit signed integer

typedef int8_t              int_fast8_t;
typedef int32_t             int_fast16_t;
typedef int32_t             int_fast32_t;
typedef int64_t             int_fast64_t;

typedef int8_t              int_least8_t;
typedef int16_t             int_least16_t;
typedef int32_t             int_least32_t;
typedef int64_t             int_least64_t;

typedef int64_t             intmax_t;                       //!< maximum-sized signed integer

#define INT8_MIN            ((int8_t)-128)                  //!< minimum int8_t value
#define INT8_MAX            ((int8_t)+127)                  //!< maximum int8_t value

#define INT16_MIN           ((int16_t)-32768)               //!< minimum int16_t value
#define INT16_MAX           ((int16_t)+32767)               //!< maximum int16_t value

#define INT32_MIN           ((int32_t)-2147483648)          //!< minimum int32_t value
#define INT32_MAX           ((int32_t)+2147483647)          //!< maximum int32_t value

#define INT64_MIN           ((int64_t)-9223372036854775808) //!< minimum int64_t value
#define INT64_MAX           ((int64_t)+9223372036854775807) //!< maximum int64_t value


#define INT_FAST8_MIN       INT8_MIN
#define INT_FAST8_MAX       INT8_MAX

#define INT_FAST16_MIN      INT16_MIN
#define INT_FAST16_MAX      INT16_MAX

#define INT_FAST32_MIN      INT32_MIN
#define INT_FAST32_MAX      INT32_MAX

#define INT_FAST64_MIN      INT64_MIN
#define INT_FAST64_MAX      INT64_MAX


#define INT_LEAST8_MIN      INT8_MIN
#define INT_LEAST8_MAX      INT8_MAX

#define INT_LEAST16_MIN     INT16_MIN
#define INT_LEAST16_MAX     INT16_MAX

#define INT_LEAST32_MIN     INT32_MIN
#define INT_LEAST32_MAX     INT32_MAX

#define INT_LEAST64_MIN     INT64_MIN
#define INT_LEAST64_MAX     INT64_MAX


#define INTMAX_MIN          INT64_MIN                       //!< minimum intmax_t value
#define INTMAX_MAX          INT64_MAX                       //!< maximum intmax_t value


//-----------------------------------------------------------------------------
// Unsigned integers
//-----------------------------------------------------------------------------
typedef unsigned char       uint8_t;                        //!< 8-bit unsigned integer
typedef unsigned short      uint16_t;                       //!< 16-bit unsigned integer
typedef unsigned int        uint32_t;                       //!< 32-bit unsigned integer
typedef unsigned long long  uint64_t;                       //!< 64-bit unsigned integer

typedef uint8_t             uint_fast8_t;
typedef uint32_t            uint_fast16_t;
typedef uint32_t            uint_fast32_t;
typedef uint64_t            uint_fast64_t;

typedef uint8_t             uint_least8_t;
typedef uint16_t            uint_least16_t;
typedef uint32_t            uint_least32_t;
typedef uint64_t            uint_least64_t;

typedef uint64_t            uintmax_t;                      //!< maximum-sized unsigned integer

#define UINT8_MAX           ((uint8_t)0xFF)                 //!< maximum uint8_t value
#define UINT16_MAX          ((uint16_t)0xFFFF)              //!< maximum uint16_t value
#define UINT32_MAX          ((uint32_t)0xFFFFFFFF)          //!< maximum uint32_t value
#define UINT64_MAX          ((uint64_t)0xFFFFFFFFFFFFFFFF)  //!< maximum uint64_t value

#define UINT_FAST8_MAX      UINT32_MAX
#define UINT_FAST16_MAX     UINT32_MAX
#define UINT_FAST32_MAX     UINT32_MAX
#define UINT_FAST64_MAX     UINT64_MAX

#define UINT_LEAST8_MAX     UINT8_MAX
#define UINT_LEAST16_MAX    UINT16_MAX
#define UINT_LEAST32_MAX    UINT32_MAX
#define UINT_LEAST64_MAX    UINT64_MAX

#define UINTMAX_MAX         UINT64_MAX                      //!< maximum uintmax_t value

//-----------------------------------------------------------------------------
// Pointer sized integers
//-----------------------------------------------------------------------------
#if NOS_PTR_SIZE == NOS_PTR_SIZE_32BIT

typedef int32_t             intptr_t;
typedef uint32_t            uintptr_t;

#define INTPTR_MIN          INT32_MIN
#define INTPTR_MAX          INT32_MAX

#define UINTPTR_MAX         UINT32_MAX

#elif NOS_PTR_SIZE == NOS_PTR_SIZE_64BIT

typedef int64_t             intptr_t;
typedef uint64_t            uintptr_t;

#define INTPTR_MIN          INT64_MIN
#define INTPTR_MAX          INT64_MAX

#define UINTPTR_MAX         UINT64_MAX

#else

#error Unsupported platform.

#endif

//! \typedef    intptr_t
//! \brief      signed integer capable of holding a pointer

//! \typedef    uintptr_t
//! \brief      unsigned integer capable of holding a pointer

//! \def    INTPTR_MIN
//! \brief  minimum intptr_t value

//! \def    INTPTR_MAX
//! \brief  maximum intptr_t value

//! \def    UINTPTR_MAX
//! \brief  maximum uintptr_t value

//-----------------------------------------------------------------------------
// Type constant macros
//-----------------------------------------------------------------------------
#define INT8_C(x)           (x)
#define INT16_C(x)          (x)
#define INT32_C(x)          (x)
#define INT64_C(x)          (x ## LL)
#define INTMAX_C(x)         INT64_C(x)

#define UINT8_C(x)          (x)
#define UINT16_C(x)         (x)
#define UINT32_C(x)         (x ## U)
#define UINT64_C(x)         (x ## ULL)
#define UINTMAX_C(x)        UINT64_C(x)

//-----------------------------------------------------------------------------
// size_t, ptrdiff_t
//-----------------------------------------------------------------------------
#if NOS_PTR_SIZE == NOS_PTR_SIZE_32BIT

typedef uint32_t    size_t;
typedef int32_t     ptrdiff_t;

typedef int32_t     __signed_size_t;
typedef uint32_t    __unsigned_ptrdiff_t;

#elif NOS_PTR_SIZE == NOS_PTR_SIZE_64BIT

typedef uint64_t    size_t;
typedef int64_t     ptrdiff_t;

typedef int64_t     __signed_size_t;
typedef uint64_t    __unsigned_ptrdiff_t;

#else
#error Unsupported platform!
#endif
