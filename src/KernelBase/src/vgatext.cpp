//-------------------------------------------------------------------------------------------------
//! \file
//! \brief  Implementation of the VGA text mode interface.
//-------------------------------------------------------------------------------------------------
#include "vgatext.h"
#include "vgaport.h"
#include "intrin.h"

NOS_EXTERN_C

//-------------------------------------------------------------------------------------------------
//! \brief  Defines some operations that can be performed on a VGA character slot.
//-------------------------------------------------------------------------------------------------
struct VgaChar
{
    uint16_t word;


    constexpr VgaChar()
        : word{ 0 }
    { }

    constexpr VgaChar(uint16_t word)
        : word{ word }
    { }

    constexpr VgaChar(uint8_t ch, uint8_t attrib)
        : word{ (uint16_t)(ch | (attrib << 8)) }
    { }

    constexpr VgaChar(uint8_t ch, VgaTextColor foreground, VgaTextColor background)
        : VgaChar{ ch, (uint8_t)(foreground | (background << 4)) }
    { }


    void Set(char ch, int color) volatile
    {
        word = (uint16_t)(ch | (color << 8));
    }

    void Set(char ch, VgaTextColor foreground, VgaTextColor background) volatile
    {
        Set(ch, (uint8_t)(foreground | (background << 4)));
    }


    void SetChar(char ch) volatile
    {
        uint16_t v = word;
        word = (uint16_t)(ch | (v & 0xFF00));
    }

    char GetChar() const volatile
    {
        return (word & 0xFF);
    }


    void SetForeground(VgaTextColor color) volatile
    {
        word = (uint16_t)((word & 0xF0FF) | (color << 8));
    }

    VgaTextColor GetForeground() const volatile
    {
        return (VgaTextColor)((word & 0x0F00) >> 8);
    }


    void SetBackground(VgaTextColor color) volatile
    {
        word = (uint16_t)((word & 0x0FFF) | (color << 12));
    }

    VgaTextColor GetBackground() const volatile
    {
        return (VgaTextColor)((word & 0xF000) >> 12);
    }


    void SetColor(VgaTextColor foreground, VgaTextColor background) volatile
    {
        word = (uint16_t)((word & 0x00FF) | (foreground | (background << 4)));
    }
};
static_assert(sizeof(VgaChar) == 2, "unexpected size of VgaChar");


// Variables
static int8_t cursorX = 0;
static int8_t cursorY = 0;


static volatile VgaChar* GetSlot(int x, int y)
{
    auto base = (volatile VgaChar*)0xB8000;
    return (base + (y * VGA_Width + x));
}

static void ScrollScreen(int lines)
{
    int x, y, n;

    for (y = 0, n = VGA_Height - lines;
        y < n;
        y++)
    {
        for (x = 0; x < VGA_Width; x++)
        {
            uint16_t prev = GetSlot(x, y + lines)->word;
            GetSlot(x, y)->word = prev;
        }
    }

    for (y = 0; y < lines; y++)
    {
        for (x = 0; x < VGA_Width; x++)
        {
            GetSlot(x, y)->word = VGA_BlankChar;
        }
    }
}

static void SyncCursorPos()
{
    uint16_t idx = static_cast<uint16_t>(cursorY * VGA_Width + cursorX);

    __outb(VGAIO_CRTControlIndex, 0x0F);
    __outb(VGAIO_CRTControlData, static_cast<uint8_t>(idx));

    __outb(VGAIO_CRTControlIndex, 0x0E);
    __outb(VGAIO_CRTControlData, static_cast<uint8_t>(idx >> 8));
}


void vtInitialize()
{
    // set the IO address selector.
    uint8_t v = __inb(VGAIO_MiscOutputIn);
    if ((v & 1) == 0)
    {
        __outb(VGAIO_MiscOutputOut, v | 1);
    }

    // get the max scan line
    __outb(VGAIO_CRTControlIndex, 0x09);
    uint8_t bottom = __inb(VGAIO_CRTControlData);
    uint8_t top = (bottom - 2) & 0x1F;

    // set the underline's start scan line
    __outb(VGAIO_CRTControlIndex, 0x0A);
    uint8_t cur = __inb(VGAIO_CRTControlData) & 0xC0;
    __outb(VGAIO_CRTControlData, cur | 0x2 | top);

    // set the underline's end scan line
    __outb(VGAIO_CRTControlIndex, 0x0B);
    cur = __inb(VGAIO_CRTControlData) & 0xE0;
    __outb(VGAIO_CRTControlData, cur | bottom);
}


void vtEnableCursor()
{
    __outb(VGAIO_CRTControlIndex, 0x0A);
    uint8_t cur = __inb(VGAIO_CRTControlData);
    __outb(VGAIO_CRTControlData, (cur & ~0x20));
}

void vtDisableCursor()
{
    __outb(VGAIO_CRTControlIndex, 0x0A);
    uint8_t cur = __inb(VGAIO_CRTControlData);
    __outb(VGAIO_CRTControlData, (cur | 0x20));
}


_Use_decl_annotations_
void vtSetCursorPos(int8_t x, int8_t y)
{
    cursorX = x;
    cursorY = y;
    SyncCursorPos();
}

_Use_decl_annotations_
void vtGetCursorPos(int* x, int* y)
{
    *x = cursorX;
    *y = cursorY;
}


void vtClearScreen()
{
    for (int y = 0; y < VGA_Height; y++)
    {
        for (int x = 0; x < VGA_Width; x++)
        {
            GetSlot(x, y)->word = VGA_BlankChar;
        }
    }

    vtSetCursorPos(0, 0);
}


void vtPrintChar(char ch, int color)
{
    char buffer[2] = { ch, 0 };
    vtPrint(buffer, color);
}

_Use_decl_annotations_
void vtPrint(const char* pszText, int color)
{
    while (*pszText != '\0')
    {
        switch (*pszText)
        {
        case '\t':
            cursorX += (4 - (cursorX % 4));
            break;

        case '\r':
            cursorX = 0;
            break;

        case '\n':
            cursorX = 0;
            cursorY++;
            break;

        default:
            GetSlot(cursorX, cursorY)->Set(*pszText, color);
            cursorX++;
            break;
        }

        if (cursorX >= VGA_Width)
        {
            cursorX = 0;
            cursorY++;
        }

        if (cursorY >= VGA_Height)
        {
            ScrollScreen(1);
            cursorY = VGA_Height - 1;
        }

        pszText++;
    }

    SyncCursorPos();
}

NOS_END_EXTERN_C