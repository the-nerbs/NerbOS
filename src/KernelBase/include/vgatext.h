//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Defines an interface to the VGA text mode capabilities.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "nosbase.h"
#include "kstdint.h"
#include "kprintf.h"
#include "sal.h"

NOS_EXTERN_C

//-------------------------------------------------------------------------------------------------
//! \brief  VGA text colors.
//!
//! \details
//! This contains 4 bit flags
//! | bit 3  | bit 2 | bit 1 | bit 0 |
//! | bright |  red  | blue  | green |
//-------------------------------------------------------------------------------------------------
_Enum_is_bitflag_
enum VgaTextColor
{
    VTC_Black = 0,

    VTC_Blue    = (1 << 0),
    VTC_Green   = (1 << 1),
    VTC_Red     = (1 << 2),
    VTC_Bright  = (1 << 3),

    VTC_Cyan        = VTC_Blue | VTC_Green,
    VTC_Magenta     = VTC_Blue | VTC_Red,
    VTC_Brown       = VTC_Red | VTC_Green,
    VTC_LightGrey   = VTC_Blue | VTC_Green | VTC_Red,

    VTC_DarkGrey        = VTC_Bright | VTC_Black,
    VTC_LightBlue       = VTC_Bright | VTC_Blue,
    VTC_LightGreen      = VTC_Bright | VTC_Green,
    VTC_LightRed        = VTC_Bright | VTC_Red,
    VTC_LightCyan       = VTC_Bright | VTC_Cyan,
    VTC_LightMagenta    = VTC_Bright | VTC_Magenta,
    VTC_Yellow          = VTC_Bright | VTC_Brown,
    VTC_White           = VTC_Bright | VTC_LightGrey,
};

#define VGA_CHAR_COLOR(fore, back) ((int8_t)((fore) | ((back) << 4)))

enum // constants
{
    // VGA mode 3 = 80 columns x 23 rows
    VGA_Width = 80,
    VGA_Height = 25,

    VGA_DefaultForeground = VTC_LightGrey,
    VGA_DefaultBackground = VTC_Black,
    VGA_DefaultColor = VGA_CHAR_COLOR(VGA_DefaultForeground, VGA_DefaultBackground),
    VGA_BlankChar = (' ' | (VGA_DefaultColor << 8)),
};




//-------------------------------------------------------------------------------------------------
//! \brief  Initializes the VGA text mode subsystem.
//-------------------------------------------------------------------------------------------------
void vtInitialize(void);


//-------------------------------------------------------------------------------------------------
//! \brief  Enables the VGA text mode cursor (the underline).
//-------------------------------------------------------------------------------------------------
void vtEnableCursor(void);

//-------------------------------------------------------------------------------------------------
//! \brief  Disables the VGA text mode cursor (the underline).
//-------------------------------------------------------------------------------------------------
void vtDisableCursor(void);


//-------------------------------------------------------------------------------------------------
//! \brief  Sets the VGA text mode cursor's position.
//!
//! \param  x, y  The new cursor position.
//-------------------------------------------------------------------------------------------------
void vtSetCursorPos(
    _In_range_(0, VGA_Width) int8_t x,
    _In_range_(0, VGA_Height) int8_t y
);

//-------------------------------------------------------------------------------------------------
//! \brief  Gets the VGA text mode cursor's position.
//!
//! \param[out]  x, y  Receives the position of the cursor.
//-------------------------------------------------------------------------------------------------
void vtGetCursorPos(
    _Out_range_(0, VGA_Width) int* x,
    _Out_range_(0, VGA_Height) int* y
);


//-------------------------------------------------------------------------------------------------
//! \brief  Clears the VGA text mode screen.
//-------------------------------------------------------------------------------------------------
void vtClearScreen(void);


//-------------------------------------------------------------------------------------------------
//! \brief  Prints a character to the VGA text mode screen.
//!
//! \param  ch     The character to print.
//! \param  color  The color to print in.
//-------------------------------------------------------------------------------------------------
void vtPrintColoredChar(char ch, int color);

inline void vtPrintChar(char ch)
{
    vtPrintColoredChar(ch, VGA_DefaultColor);
}

//-------------------------------------------------------------------------------------------------
//! \brief  Prints a string of characters to the VGA text mode screen.
//!
//! \param  pszText  The string to print.
//! \param  color    The color to print in.
//-------------------------------------------------------------------------------------------------
void vtPrintColoredString(_In_z_ const char* pszText, int color);

inline void vtPrintString(_In_z_ const char* pszText)
{
    vtPrintColoredString(pszText, VGA_DefaultColor);
}

//-------------------------------------------------------------------------------------------------
//! \brief  Gets a kprintf stream which prints to the VGA text mode screen.
//-------------------------------------------------------------------------------------------------
const kprintf_stream* vtKPrintfStream(void);

NOS_END_EXTERN_C
