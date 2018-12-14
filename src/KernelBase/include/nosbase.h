#pragma once

#ifdef __cplusplus

#define NOS_EXTERN_C        extern "C" {
#define NOS_END_EXTERN_C    }

#else

#define NOS_EXTERN_C        
#define NOS_END_EXTERN_C    

typedef signed short        wchar_t;

#endif

#define NOS_UNUSED_PARAM(x) ((void)x)
