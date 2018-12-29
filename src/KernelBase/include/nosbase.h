#pragma once

#ifdef __cplusplus

// C++ stuff
#define NOS_EXTERN_C        extern "C" {
#define NOS_END_EXTERN_C    }

#else

// C stuff
#define NOS_EXTERN_C        
#define NOS_END_EXTERN_C    

typedef signed short        wchar_t;

typedef _Bool               bool;
#define true                1
#define false               0

#endif

#define COUNTOF(arr)    ((sizeof(arr)) / sizeof(*arr))

#define NOS_UNUSED_PARAM(x) ((void)x)
