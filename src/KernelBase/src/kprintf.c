#include "kprintf.h"
#include "nosbase.h"
#include "kstddef.h"
#include "intrin.h"
#include "sal.h"

// note: throughout this file, comments starting with 3 slashes instead
// of 2 (`///`) are used to indicate direct quotes from the C11 spec.

//-------------------------------------------------------------------------------------------------
// constants
//-------------------------------------------------------------------------------------------------
static const char LowerDigits[] = { '0','1','2','3','4','5','6','7','8','9', 'a', 'b', 'c', 'd', 'e', 'f' };
static const char UpperDigits[] = { '0','1','2','3','4','5','6','7','8','9', 'A', 'B', 'C', 'D', 'E', 'F' };

enum //constants
{
    IntegerBufLen = 64,
};


//-------------------------------------------------------------------------------------------------
// types
//-------------------------------------------------------------------------------------------------
typedef enum tagFormatType
{
    FT_Empty,               // no specifier parsed yet, or we encountered an invalid specifier.
    FT_Percent,             // %%
    FT_Character,           // %c
    FT_String,              // %s
    FT_Decimal,             // %d, %i
    FT_OctalInt,            // %o
    FT_HexLower,            // %x
    FT_HexUpper,            // %X
    FT_Unsigned,            // %u

#if NOS_FLOAT_SUPPORT
    FT_FloatLower,          // %f
    FT_FloatUpper,          // %F
    FT_ExpFloatLower,       // %e
    FT_ExpFloatUpper,       // %E
    FT_HexFloatLower,       // %a
    FT_HexFloatUpper,       // %A
    FT_GeneralFloatLower,   // %g
    FT_GeneralFloatUpper,   // %G
#endif // NOS_FLOAT_SUPPORT

    FT_Pointer,             // %p
} FormatType;

typedef enum tagFormatFlags
{
    FF_None         = 0,
    FF_LeftJustify  = (1 << 0),     // '-'
    FF_PrintSign    = (1 << 1),     // '+'
    FF_PadSign      = (1 << 2),     // ' '
    FF_AltForm      = (1 << 3),     // '#'
    FF_ZeroPad      = (1 << 4),     // '0'
} FormatFlags;

typedef enum tagFormatWidth
{
    FW_Default = 0,
    FW_hh,
    FW_h,
    FW_l,
    FW_ll,
    FW_j,
    FW_z,
    FW_t,
    FW_L,
} FormatWidth;

typedef enum tagPadPrecisionKind
{
    PK_Default = 0,
    PK_Given = 1,
    PK_FromArg = 2,
} PadPrecisionKind;

typedef struct tag_format_spec
{
    uint8_t type;
    uint8_t flags;
    uint8_t width;
    uint8_t paddingAndPrecisionKinds;
    int padding;
    int precision;
} format_spec;

#if NOS_FLOAT_SUPPORT
typedef long double floatmax_t;
#endif // NOS_FLOAT_SUPPORT


//-------------------------------------------------------------------------------------------------
// format_spec interactions
//-------------------------------------------------------------------------------------------------
inline PadPrecisionKind pad_kind(_In_ const format_spec* spec)
{
    return (spec->paddingAndPrecisionKinds & 0x0f);
}

inline void set_pad_kind(_In_ format_spec* spec, PadPrecisionKind kind)
{
    int8_t pk = spec->paddingAndPrecisionKinds & 0xf0;
    spec->paddingAndPrecisionKinds = (uint8_t)(pk | kind);
}

inline PadPrecisionKind precision_kind(_In_ const format_spec* spec)
{
    return ((spec->paddingAndPrecisionKinds & 0xf0) >> 4);
}

inline void set_precision_kind(_In_ format_spec* spec, PadPrecisionKind kind)
{
    int8_t pk = spec->paddingAndPrecisionKinds & 0x0f;
    spec->paddingAndPrecisionKinds = (uint8_t)(pk | (kind << 4));
}

inline bool has_flag(_In_ const format_spec* spec, FormatFlags flags)
{
    return ((spec->flags & flags) == flags);
}


//-------------------------------------------------------------------------------------------------
// format spec parsing
//-------------------------------------------------------------------------------------------------
static void parse_format_flags(_In_ const char** fmt, _Inout_ format_spec* spec)
{
    for (;;)
    {
        switch (**fmt)
        {
        case '-':
            spec->flags |= FF_LeftJustify;
            break;

        case '+':
            spec->flags |= FF_PrintSign;
            break;

        case ' ':
            spec->flags |= FF_PadSign;
            break;

        case '#':
            spec->flags |= FF_AltForm;
            break;

        case '0':
            spec->flags |= FF_ZeroPad;
            break;

        default:
            // we reached the end of the flags
            if (has_flag(spec, FF_PadSign)
                && has_flag(spec, FF_PrintSign))
            {
                // If the space and + flags both appear, the space flag is ignored.
                spec->flags &= ~FF_PadSign;
            }

            if (has_flag(spec, FF_ZeroPad)
                && has_flag(spec, FF_LeftJustify))
            {
                // If the 0 and - flags both appear, the 0 flag is ignored.
                spec->flags &= FF_ZeroPad;
            }
            return;
        }

        (*fmt)++;
    }
}

static void parse_format_padding(_In_ const char** fmt, _Inout_ format_spec* spec)
{
    if (**fmt == '*')
    {
        set_pad_kind(spec, PK_FromArg);
        (*fmt)++;
    }
    else if ('0' <= **fmt && **fmt <= '9')
    {
        set_pad_kind(spec, PK_Given);
        spec->padding = 0;

        do
        {
            spec->padding = 10 * spec->padding + (**fmt - '0');
            (*fmt)++;
        } while ('0' <= **fmt && **fmt <= '9');
    }
    else
    {
        set_pad_kind(spec, PK_Default);
    }
}

static void parse_format_precision(_In_ const char** fmt, _Inout_ format_spec* spec)
{
    if (**fmt == '.')
    {
        if (**fmt == '*')
        {
            set_precision_kind(spec, PK_FromArg);
            (*fmt)++;
        }
        else if ('0' <= **fmt && **fmt <= '9')
        {
            set_precision_kind(spec, PK_Given);
            spec->precision = 0;

            do
            {
                spec->precision = 10 * spec->precision + (**fmt - '0');
                (*fmt)++;
            } while ('0' <= **fmt && **fmt <= '9');
        }
        else
        {
            set_precision_kind(spec, PK_Default);
        }
    }
}

static void parse_format_width(_In_ const char** fmt, _Inout_ format_spec* spec)
{
    switch (**fmt)
    {
    case 'h':
        if (*((*fmt) + 1) == 'h')
        {
            spec->width = FW_hh;
            *fmt += 2;
        }
        else
        {
            spec->width = FW_h;
            (*fmt) += 1;
        }
        break;

    case 'l':
        if (*((*fmt) + 1) == 'l')
        {
            spec->width = FW_ll;
            (*fmt) += 2;
        }
        else
        {
            spec->width = FW_l;
            (*fmt) += 1;
        }
        break;

    case 'j':
        spec->width = FW_j;
        (*fmt) += 1;
        break;

    case 'z':
        spec->width = FW_z;
        (*fmt) += 1;
        break;

    case 't':
        spec->width = FW_t;
        (*fmt) += 1;
        break;

    case 'L':
        spec->width = FW_L;
        (*fmt) += 1;
        break;

    default:
        break;
    }
}

static void parse_format_type(_In_ const char** fmt, _Inout_ format_spec* spec)
{
    switch (**fmt)
    {
    case '%':
        spec->type = FT_Percent;
        break;

    case 'c':
        spec->type = FT_Character;
        break;

    case 's':
        spec->type = FT_String;
        break;

    case 'd':
    case 'i':
        spec->type = FT_Decimal;
        break;

    case 'o':
        spec->type = FT_OctalInt;
        break;

    case 'x':
        spec->type = FT_HexLower;
        break;

    case 'X':
        spec->type = FT_HexUpper;
        break;

    case 'u':
        spec->type = FT_Unsigned;
        break;

#if NOS_FLOAT_SUPPORT

    case 'f':
        spec->type = FT_FloatLower;
        break;

    case 'F':
        spec->type = FT_FloatUpper;
        break;

    case 'e':
        spec->type = FT_ExpFloatLower;
        break;

    case 'E':
        spec->type = FT_ExpFloatUpper;
        break;

    case 'a':
        spec->type = FT_HexFloatLower;
        break;

    case 'A':
        spec->type = FT_HexFloatUpper;
        break;

    case 'g':
        spec->type = FT_GeneralFloatLower;
        break;

    case 'G':
        spec->type = FT_GeneralFloatUpper;
        break;

#endif // NOS_FLOAT_SUPPORT

    case 'p':
        spec->type = FT_Pointer;
        break;

    default:
        // invalid type - just skip it.
        spec->type = FT_Empty;
        break;
    }

    // advance past the format type spec.
    (*fmt)++;
}

static bool is_valid_spec(_In_ const format_spec* spec)
{
    switch (spec->type)
    {
    case FT_Percent:
        return spec->flags == FF_None
            && spec->paddingAndPrecisionKinds == 0
            && pad_kind(spec) != PK_Default
            && precision_kind(spec) != PK_Default
            && spec->width == FW_Default;

    case FT_Character:
        return spec->width == FW_Default        // int
#if NOS_PRINTF_WCHAR_SUPPORT
            || spec->width == FW_l              // wint_t
#endif // NOS_PRINTF_WCHAR_SUPPORT
            ;

    case FT_String:
        return spec->width == FW_Default        // char*
#if NOS_PRINTF_WCHAR_SUPPORT
            || spec->width == FW_l              // wchar_t*
#endif // NOS_PRINTF_WCHAR_SUPPORT
            ;

    case FT_Decimal:
        return spec->width == FW_hh             // signed char
            || spec->width == FW_h              // short
            || spec->width == FW_Default        // int
            || spec->width == FW_l              // long
            || spec->width == FW_ll             // long long
            || spec->width == FW_j              // intmax_t
            || spec->width == FW_z              // signed size_t
            || spec->width == FW_t;             // ptrdiff_t

    case FT_OctalInt:
    case FT_HexLower:
    case FT_HexUpper:
    case FT_Unsigned:
        return spec->width == FW_hh             // unsigned char
            || spec->width == FW_h              // unsigned short
            || spec->width == FW_Default        // unsigned int
            || spec->width == FW_l              // unsigned long
            || spec->width == FW_ll             // unsigned long long
            || spec->width == FW_j              // uintmax_t
            || spec->width == FW_z              // size_t
            || spec->width == FW_t;             // unsigned ptrdiff_t

#if NOS_FLOAT_SUPPORT

    case FT_FloatLower:
    case FT_FloatUpper:
    case FT_ExpFloatLower:
    case FT_ExpFloatUpper:
    case FT_HexFloatLower:
    case FT_HexFloatUpper:
    case FT_GeneralFloatLower:
    case FT_GeneralFloatUpper:
        return spec->width == FW_Default        // double
            || spec->width == FW_l              // double
            || spec->width == FW_L;             // long double

#endif // NOS_FLOAT_SUPPORT

    case FT_Pointer:
        return spec->width == FW_Default;       // void*

    default:
        return false;
    }
}

static void parse_format_spec(_In_ const char** fmt, _Out_ format_spec* spec)
{
    memset(spec, 0, sizeof(*spec));

    // skip initial '%'
    (*fmt)++;

    if (**fmt == '%')
    {
        spec->type = FT_Percent;

        // skip the specifier '%'
        (*fmt)++;
    }
    else
    {
        parse_format_flags(fmt, spec);
        parse_format_padding(fmt, spec);
        parse_format_precision(fmt, spec);
        parse_format_width(fmt, spec);
        parse_format_type(fmt, spec);
    }

    if (!is_valid_spec(spec))
    {
        // ignore this specifier
        spec->type = FT_Empty;
    }
}


//-------------------------------------------------------------------------------------------------
// helpers
//-------------------------------------------------------------------------------------------------
static intmax_t read_signed(_In_ const format_spec* spec, _Inout_ va_list* args)
{
    switch (spec->width)
    {
    default:
    case FW_Default:
        return va_arg(*args, int);

    case FW_hh:
        return va_arg(*args, signed char);

    case FW_h:
        return va_arg(*args, short);

    case FW_l:
        return va_arg(*args, long);

    case FW_ll:
        return va_arg(*args, long long);

    case FW_j:
        return va_arg(*args, intmax_t);

    case FW_z:
        return va_arg(*args, __signed_size_t);

    case FW_t:
        return va_arg(*args, ptrdiff_t);
    }
}

static uintmax_t read_unsigned(_In_ const format_spec* spec, _Inout_ va_list* args)
{
    switch (spec->width)
    {
    default:
    case FW_Default:
        return va_arg(*args, unsigned int);

    case FW_hh:
        return va_arg(*args, unsigned char);

    case FW_h:
        return va_arg(*args, unsigned short);

    case FW_l:
        return va_arg(*args, unsigned long);

    case FW_ll:
        return va_arg(*args, unsigned long long);

    case FW_j:
        return va_arg(*args, uintmax_t);

    case FW_z:
        return va_arg(*args, size_t);

    case FW_t:
        return va_arg(*args, __unsigned_ptrdiff_t);
    }
}

#if NOS_FLOAT_SUPPORT
static floatmax_t read_float(_In_ const format_spec* spec, _Inout_ va_list* args)
{
    switch (spec->width)
    {
    default:
    case FW_Default:
        return va_arg(*args, double);

    case FW_L:
        return va_arg(*args, long double);
    }
}
#endif // NOS_FLOAT_SUPPORT

inline int get_format_padding(_In_ const format_spec* spec, _In_ va_list* args, int defaultPad)
{
    NOS_UNUSED_PARAM(args);

    switch (pad_kind(spec))
    {
    case PK_Default:
    case PK_FromArg:            // handled in kvprintf
        return defaultPad;

    case PK_Given:
        return spec->padding;

    default:
        //TODO: kassert(false)
        return defaultPad;
    }
}

inline int get_format_precision(_In_ const format_spec* spec, _In_ va_list* args, int defaultPrecision)
{
    NOS_UNUSED_PARAM(args);

    switch (precision_kind(spec))
    {
    case PK_Default:
    case PK_FromArg:            // handled in kvprintf
        return defaultPrecision;

    case PK_Given:
        return spec->precision;

    default:
        //TODO: kassert(false)
        return defaultPrecision;
    }
}

inline char get_int_sign_char(intmax_t value, _In_ const format_spec* spec)
{
    if (value < 0)
    {
        return '-';
    }

    if (has_flag(spec, FF_PrintSign))
    {
        return '+';
    }

    if (has_flag(spec, FF_PadSign))
    {
        return ' ';
    }

    return 0;
}


static char* signed_to_string(
    intmax_t value,
    _In_count_(bufLen) char* buffer,
    int bufLen,
    _In_range_(2,16) int radix,
    int precision,
    _In_ const format_spec* spec)
{
    //note: HexUpper is the only uppercase integer format type.
    const char* pDigits = (spec->type == FT_HexUpper) ? UpperDigits : LowerDigits;

    char* pBuf = buffer + bufLen - 1;
    *pBuf = 0;

    while (pBuf >= buffer
        && (value != 0 || precision > 0))
    {
        int digit = abs((int)(value % radix));
        value /= radix;

        pBuf--;
        *pBuf = pDigits[digit];

        precision--;
    }

    return pBuf;
}

static char* unsigned_to_string(
    uintmax_t value,
    _In_count_(bufLen) char *buffer,
    int bufLen,
    _In_range_(2, 16) int radix,
    int precision,
    _In_ const format_spec* spec)
{
    //note: HexUpper is the only uppercase integer format type.
    const char* pDigits = (spec->type == FT_HexUpper) ? UpperDigits : LowerDigits;

    char* pBuf = buffer + bufLen - 1;
    *pBuf = 0;

    while (value > 0 || precision > 0)
    {
        int digit = value % radix;
        value /= radix;

        pBuf--;
        *pBuf = pDigits[digit];

        precision--;
    }

    return pBuf;
}


inline void write_chars(_In_ const kprintf_stream* stream, char ch, int count)
{
    for (int i = 0; i < count; i++)
    {
        stream->write(ch);
    }
}

inline void write_string_length(_In_ const kprintf_stream* stream, _In_z_ const char* psz, int length)
{
    for (int i = 0;
         *psz != 0 && i < length;
         i++, psz++)
    {
        stream->write(*psz);
    }
}

inline void write_string(_In_ const kprintf_stream* stream, _In_z_ const char* psz)
{
    for (; *psz != 0; psz++)
    {
        stream->write(*psz);
    }
}

static void output_string(_In_ const kprintf_stream* stream, _In_ const char* psz, _In_ const format_spec* spec)
{
    const int len = (int)strlen(psz);

    const int valueLen = precision_kind(spec) == PK_Default
        ? len
        : MIN(len, spec->precision);

    const int padSize = spec->padding - (int)valueLen;

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }

    write_string_length(stream, psz, valueLen);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}

//-------------------------------------------------------------------------------------------------
// printing functions
//-------------------------------------------------------------------------------------------------
static void format_percent(_In_ const kprintf_stream* stream)
{
    stream->write('%');
}

static void format_character(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
#if NOS_PRINTF_WCHAR_SUPPORT

    if (spec->width == FW_l)
    {
        //TODO - wchar_t support
    }
    else

#endif  // NOS_PRINTF_WCHAR_SUPPORT

    {
        char buffer[2];
        buffer[0] = (char)va_arg(*ap, int);
        buffer[1] = 0;

        output_string(stream, buffer, spec);
    }
}

static void format_string(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
#if NOS_PRINTF_WCHAR_SUPPORT

    if (spec->width == FW_l)
    {
        //TODO - wchar_t support
    }
    else

#endif  // NOS_PRINTF_WCHAR_SUPPORT

    {
        const char* psz = va_arg(*ap, char*);
        output_string(stream, psz, spec);
    }
}

static void format_decimal(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
    const int width         = get_format_padding(spec, ap, 0);
    const int precision     = get_format_precision(spec, ap, 1);    /// The default precision is 1
    const intmax_t value    = read_signed(spec, ap);

    // '+' flag:
    /// The result of a signed conversion always begins with a plus or minus sign. (It begins
    /// with a sign only when a negative value is converted if this flag is not specified.)
    //
    // ' ' flag:
    /// If the first character of a signed conversion is not a sign, or if a signed conversion
    /// results in no characters, a space is prefixed to the result. If the space and + flags
    /// both appear, the space flag is ignored.
    //
    // sign = '\0' used to indicate that the sign is not printed.
    const char sign = get_int_sign_char(value, spec);
    const bool printSign = sign != '\0';

    /// The result of converting a zero value with a precision of zero is no characters.
    const bool printNumber = value != 0
        || precision_kind(spec) != PK_Default
        || precision != 0;

    char buffer[IntegerBufLen];
    memset(buffer, 0, sizeof(buffer));

    char* pszValue = printNumber
        ? signed_to_string(value, buffer, COUNTOF(buffer), 10, precision, spec)
        : &(buffer[COUNTOF(buffer) - 1]);

    const int valueLen = (int)strlen(pszValue);
    const int padSize = width - valueLen - (printSign ? 1 : 0);

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        // If the '0' flag is specified:
        /// leading zeros (following any indication of sign or base) are used to pad to the field
        /// width rather than performing space padding [...]. If the 0 and - flags both appear,
        /// the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a precision is 
        /// specified, the 0 flag is ignored.
        if (has_flag(spec, FF_ZeroPad)
            && precision_kind(spec) == PK_Default)
        {
            if (printSign)
            {
                stream->write(sign);
            }

            write_chars(stream, '0', padSize);
        }
        else
        {
            stream->write(' ');

            if (printSign)
            {
                stream->write(sign);
            }
        }
    }
    else if (printSign)
    {
        stream->write(sign);
    }

    write_string(stream, pszValue);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}

static void format_octal(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
    const int width         = get_format_padding(spec, ap, 0);
    const int precision     = get_format_precision(spec, ap, 1);    /// The default precision is 1
    const uintmax_t value   = read_unsigned(spec, ap);

    /// The result of converting a zero value with a precision of zero is no characters.
    const bool printNumber = value != 0
        || precision_kind(spec) != PK_Default
        || precision != 0;

    char buffer[IntegerBufLen];
    memset(buffer, 0, sizeof(buffer));

    char* pszValue = printNumber
        ? unsigned_to_string(value, buffer, COUNTOF(buffer), 8, precision, spec)
        : &(buffer[COUNTOF(buffer) - 1]);

    const int valueLen = (int)strlen(pszValue);

    /// For o conversion, [the alternative form flag] increases the precision, if and only if
    /// necessary, to force the first digit of the result to be a zero (if the value and
    /// precision are both 0, a single 0 is printed)
    const bool printBase = has_flag(spec, FF_AltForm) && valueLen > 0 && (*pszValue != '0');
    const int padSize = width - valueLen - (printBase ? 1 : 0);

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        // If the '0' flag is specified:
        /// leading zeros (following any indication of sign or base) are used to pad to the field
        /// width rather than performing space padding [...]. If the 0 and - flags both appear,
        /// the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a precision is 
        /// specified, the 0 flag is ignored.
        if (has_flag(spec, FF_ZeroPad)
            && precision_kind(spec) == PK_Default)
        {
            if (printBase)
            {
                // base indicator
                stream->write('0');
            }

            // zero padding
            write_chars(stream, '0', padSize);
        }
        else
        {
            // space padding
            write_chars(stream, ' ', padSize);

            /// (see alternative form flag above)
            if (printBase)
            {
                // base indicator
                stream->write('0');
            }
        }
    }
    else if (printBase)
    {
        /// (see alternative form flag above)
        stream->write('0');
    }

    write_string(stream, pszValue);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}

static void format_hex(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
    const int width         = get_format_padding(spec, ap, 0);
    const int precision     = get_format_precision(spec, ap, 1);    /// The default precision is 1
    const uintmax_t value   = read_unsigned(spec, ap);

    /// For x (or X) conversion, a nonzero result has 0x (or 0X) prefixed to it.
    const bool altForm = has_flag(spec, FF_AltForm);
    const bool printBase = altForm && value != 0;

    /// The result of converting a zero value with a precision of zero is no characters.
    const bool printNumber = value != 0
        || precision_kind(spec) == PK_Default
        || precision != 0;

    char buffer[IntegerBufLen];
    memset(buffer, 0, sizeof(buffer));

    char* pszValue = printNumber
        ? unsigned_to_string(value, buffer, COUNTOF(buffer), 16, precision, spec)
        : &(buffer[COUNTOF(buffer) - 1]);

    const int valueLen = (int)strlen(pszValue);
    const int padSize = width - valueLen - (printBase ? 2 : 0);

    const char x = spec->type == FT_HexUpper ? 'X' : 'x';

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        // If the '0' flag is specified:
        /// leading zeros (following any indication of sign or base) are used to pad to the field
        /// width rather than performing space padding [...]. If the 0 and - flags both appear,
        /// the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a precision is 
        /// specified, the 0 flag is ignored.
        if (has_flag(spec, FF_ZeroPad)
            && precision_kind(spec) == PK_Default)
        {
            if (printBase)
            {
                // base indicator
                stream->write('0');
                stream->write(x);
            }

            // zero padding
            write_chars(stream, '0', padSize);
        }
        else
        {
            // space padding
            write_chars(stream, ' ', padSize);

            if (printBase)
            {
                // base indicator
                stream->write('0');
                stream->write(x);
            }
        }
    }
    else if (printBase)
    {
        // base indicator
        stream->write('0');
        stream->write(x);
    }

    write_string(stream, pszValue);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}

static void format_unsigned(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
    const int width         = get_format_padding(spec, ap, 0);
    const int precision     = get_format_precision(spec, ap, 1);    /// The default precision is 1
    const uintmax_t value   = read_unsigned(spec, ap);

    /// The result of converting a zero value with a precision of zero is no characters.
    const bool printNumber = value != 0
        || precision_kind(spec) == PK_Default
        || precision != 0;

    char buffer[IntegerBufLen];
    memset(buffer, 0, sizeof(buffer));

    char* pszValue = printNumber
        ? unsigned_to_string(value, buffer, COUNTOF(buffer), 10, precision, spec)
        : &(buffer[COUNTOF(buffer) - 1]);

    const int valueLen = (int)strlen(pszValue);
    const int padSize = width - valueLen;

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        // If the '0' flag is specified:
        /// leading zeros (following any indication of sign or base) are used to pad to the field
        /// width rather than performing space padding [...]. If the 0 and - flags both appear,
        /// the 0 flag is ignored. For d, i, o, u, x, and X conversions, if a precision is 
        /// specified, the 0 flag is ignored.
        if (has_flag(spec, FF_ZeroPad)
            && precision_kind(spec) == PK_Default)
        {
            // zero padding
            write_chars(stream, '0', padSize);
        }
        else
        {
            // space padding
            write_chars(stream, ' ', padSize);
        }
    }

    write_string(stream, pszValue);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}

static void format_pointer(_In_ const kprintf_stream* stream, _In_ const format_spec* spec, _In_ va_list* ap)
{
    const int width = get_format_padding(spec, ap, 0);

    // we don't use precision for pointers, but it is still valid to have one per the C11 spec
    get_format_precision(spec, ap, 0);

    const uintptr_t value = (uintptr_t)va_arg(*ap, void*);


    char buffer[IntegerBufLen];
    memset(buffer, 0, sizeof(buffer));

    char* pszVal = unsigned_to_string(value, buffer, COUNTOF(buffer), 16, 2 * sizeof(void*), spec);

    // prepend a '0x'
    pszVal--;
    *pszVal = 'x';

    pszVal--;
    *pszVal = '0';

    const int len = (int)strlen(pszVal);
    const int padSize = width - len;

    if (padSize > 0
        && !has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }

    write_string(stream, pszVal);

    if (padSize > 0
        && has_flag(spec, FF_LeftJustify))
    {
        write_chars(stream, ' ', padSize);
    }
}



//-------------------------------------------------------------------------------------------------
// interface implementations
//-------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void kprintf(const kprintf_stream* stream, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    kvprintf(stream, fmt, ap);

    va_end(ap);
}

_Use_decl_annotations_
void kvprintf(const kprintf_stream* stream, const char* fmt, va_list args)
{
    if (stream == NULL
        || stream->write == NULL)
    {
        //TODO: kassert(false)
        return;
    }

    while (*fmt != 0)
    {
        while (*fmt != '%' && *fmt != 0)
        {
            stream->write(*fmt);
            fmt++;
        }

        if (*fmt == '%')
        {
            format_spec spec;
            parse_format_spec(&fmt, &spec);

            if (pad_kind(&spec) == PK_FromArg)
            {
                int width = va_arg(args, int);

                /// A negative field width argument is taken as a - flag followed by a
                /// positive field width.
                if (width < 0)
                {
                    spec.flags |= FF_LeftJustify;
                    width = -width;
                }

                spec.padding = width;
                set_pad_kind(&spec, PK_Given);
            }

            if (precision_kind(&spec) == PK_FromArg)
            {
                int precision = va_arg(args, int);

                /// A negative precision argument is taken as if the precision were omitted.
                if (precision >= 0)
                {
                    spec.precision = precision;
                    set_precision_kind(&spec, PK_Given);
                }
                else
                {
                    set_precision_kind(&spec, PK_Default);
                }
            }

            switch (spec.type)
            {
            case FT_Percent:             // %%
                format_percent(stream);
                break;

            case FT_Character:           // %c
                format_character(stream, &spec, &args);
                break;

            case FT_String:              // %s
                format_string(stream, &spec, &args);
                break;

            case FT_Decimal:             // %d, %i
                format_decimal(stream, &spec, &args);
                break;

            case FT_OctalInt:            // %o
                format_octal(stream, &spec, &args);
                break;

            case FT_HexLower:            // %x
            case FT_HexUpper:            // %X
                format_hex(stream, &spec, &args);
                break;

            case FT_Unsigned:            // %u
                format_unsigned(stream, &spec, &args);
                break;

#if NOS_FLOAT_SUPPORT
            case FT_FloatLower:          // %f
            case FT_FloatUpper:          // %F
            case FT_ExpFloatLower:       // %e
            case FT_ExpFloatUpper:       // %E
            case FT_HexFloatLower:       // %a
            case FT_HexFloatUpper:       // %A
            case FT_GeneralFloatLower:   // %g
            case FT_GeneralFloatUpper:   // %G
                //TODO: float support
#endif // NOS_FLOAT_SUPPORT
                //TODO: kassert(false)
                break;

            case FT_Pointer:             // %p
                format_pointer(stream, &spec, &args);
                break;

            default:
            case FT_Empty:               // we got an invalid specifier.
                //TODO: kassert(false)
                break;
            }
        }
    }
}
