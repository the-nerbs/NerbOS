//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Includes MSVC's Source Annotation Headers
//-------------------------------------------------------------------------------------------------
#pragma once

#if     defined(_MSC_VER)

#include "msvc\sal.h"

#else   // defined(_MSC_VER)

#include "msvc\no_sal2.h"

#endif  // defined(_MSC_VER)
