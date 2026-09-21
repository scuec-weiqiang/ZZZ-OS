#include <os/types.h>
#include <os/utils.h>
#include <os/vsprintf.h>
#include <stdarg.h>

enum format_flags {
    FORMAT_LEFT    = 1U << 0,
    FORMAT_PLUS    = 1U << 1,
    FORMAT_SPACE   = 1U << 2,
    FORMAT_SPECIAL = 1U << 3,
    FORMAT_ZEROPAD = 1U << 4,
    FORMAT_PREFIX_ZERO = 1U << 5,
};

enum length_qualifier {
    QUAL_NONE,
    QUAL_HH,
    QUAL_H,
    QUAL_L,
    QUAL_LL,
    QUAL_Z,
    QUAL_T,
};

struct print_output {
    char *buf;
    size_t size;
    size_t count;
};

static void output_char(struct print_output *out, char c)
{
    if (out->size && out->count < out->size - 1)
        out->buf[out->count] = c;
    out->count++;
}

static void output_repeat(struct print_output *out, char c, int count)
{
    while (count-- > 0)
        output_char(out, c);
}

static int decimal_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int parse_decimal(const char **fmt)
{
    int value = 0;

    while (decimal_digit(**fmt)) {
        int digit = **fmt - '0';

        if (value > (INT_MAX - digit) / 10)
            value = INT_MAX;
        else
            value = value * 10 + digit;
        (*fmt)++;
    }
    return value;
}

static unsigned long long get_unsigned_arg(va_list *args,
                                            enum length_qualifier qualifier)
{
    switch (qualifier) {
    case QUAL_HH:
        return (unsigned char)va_arg(*args, unsigned int);
    case QUAL_H:
        return (unsigned short)va_arg(*args, unsigned int);
    case QUAL_L:
        return va_arg(*args, unsigned long);
    case QUAL_LL:
        return va_arg(*args, unsigned long long);
    case QUAL_Z:
        return va_arg(*args, size_t);
    case QUAL_T:
        return va_arg(*args, uintptr_t);
    case QUAL_NONE:
    default:
        return va_arg(*args, unsigned int);
    }
}

static long long get_signed_arg(va_list *args,
                                enum length_qualifier qualifier)
{
    switch (qualifier) {
    case QUAL_HH:
        return (signed char)va_arg(*args, int);
    case QUAL_H:
        return (short)va_arg(*args, int);
    case QUAL_L:
        return va_arg(*args, long);
    case QUAL_LL:
        return va_arg(*args, long long);
    case QUAL_Z:
        return va_arg(*args, ssize_t);
    case QUAL_T:
        return va_arg(*args, intptr_t);
    case QUAL_NONE:
    default:
        return va_arg(*args, int);
    }
}

static void format_number(struct print_output *out, unsigned long long value,
                          int negative, unsigned int base, int uppercase,
                          unsigned int flags, int width, int precision)
{
    const char *digits = uppercase ? "0123456789ABCDEF" :
                                     "0123456789abcdef";
    char reversed[sizeof(value) * 8];
    char sign = 0;
    char prefix1 = 0;
    char prefix2 = 0;
    int digit_count = 0;
    int zero_count;
    int padding;

    if (negative)
        sign = '-';
    else if (flags & FORMAT_PLUS)
        sign = '+';
    else if (flags & FORMAT_SPACE)
        sign = ' ';

    if ((flags & FORMAT_SPECIAL) && base == 16 &&
        (value != 0 || (flags & FORMAT_PREFIX_ZERO))) {
        prefix1 = '0';
        prefix2 = uppercase ? 'X' : 'x';
    }

    if (value == 0) {
        if (precision != 0)
            reversed[digit_count++] = '0';
    } else {
        while (value) {
            unsigned int remainder;

            value = divmod_u64(value, base, &remainder);
            reversed[digit_count++] = digits[remainder];
        }
    }

    /* %#o must begin with one zero, including the %#.0o case. */
    if ((flags & FORMAT_SPECIAL) && base == 8 &&
        (digit_count == 0 || reversed[digit_count - 1] != '0')) {
        if (precision <= digit_count)
            precision = digit_count + 1;
    }

    zero_count = precision > digit_count ? precision - digit_count : 0;
    padding = width - digit_count - zero_count;
    if (sign)
        padding--;
    if (prefix1)
        padding--;
    if (prefix2)
        padding--;
    if (padding < 0)
        padding = 0;

    if (!(flags & FORMAT_LEFT) &&
        (!(flags & FORMAT_ZEROPAD) || precision >= 0))
        output_repeat(out, ' ', padding);

    if (sign)
        output_char(out, sign);
    if (prefix1)
        output_char(out, prefix1);
    if (prefix2)
        output_char(out, prefix2);

    if (!(flags & FORMAT_LEFT) && (flags & FORMAT_ZEROPAD) && precision < 0)
        output_repeat(out, '0', padding);

    output_repeat(out, '0', zero_count);
    while (digit_count > 0)
        output_char(out, reversed[--digit_count]);

    if (flags & FORMAT_LEFT)
        output_repeat(out, ' ', padding);
}

static void format_string(struct print_output *out, const char *str,
                          unsigned int flags, int width, int precision)
{
    int length = 0;
    int padding;

    if (!str)
        str = "(null)";

    while (str[length] && (precision < 0 || length < precision))
        length++;

    padding = width > length ? width - length : 0;
    if (!(flags & FORMAT_LEFT))
        output_repeat(out, ' ', padding);
    for (int i = 0; i < length; i++)
        output_char(out, str[i]);
    if (flags & FORMAT_LEFT)
        output_repeat(out, ' ', padding);
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    struct print_output out = {
        .buf = buf,
        .size = size,
        .count = 0,
    };
    va_list ap;

    if (!fmt || (size && !buf))
        return 0;

    va_copy(ap, args);
    while (*fmt) {
        unsigned int flags = 0;
        enum length_qualifier qualifier = QUAL_NONE;
        int width = -1;
        int precision = -1;
        char conversion;

        if (*fmt != '%') {
            output_char(&out, *fmt++);
            continue;
        }
        fmt++;

        for (;;) {
            if (*fmt == '-')
                flags |= FORMAT_LEFT;
            else if (*fmt == '+')
                flags |= FORMAT_PLUS;
            else if (*fmt == ' ')
                flags |= FORMAT_SPACE;
            else if (*fmt == '#')
                flags |= FORMAT_SPECIAL;
            else if (*fmt == '0')
                flags |= FORMAT_ZEROPAD;
            else
                break;
            fmt++;
        }

        if (*fmt == '*') {
            width = va_arg(ap, int);
            fmt++;
            if (width < 0) {
                flags |= FORMAT_LEFT;
                width = width == (-INT_MAX - 1) ? INT_MAX : -width;
            }
        } else if (decimal_digit(*fmt)) {
            width = parse_decimal(&fmt);
        }

        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                precision = va_arg(ap, int);
                fmt++;
                if (precision < 0)
                    precision = -1;
            } else {
                precision = parse_decimal(&fmt);
            }
        }

        if (*fmt == 'h') {
            fmt++;
            qualifier = QUAL_H;
            if (*fmt == 'h') {
                qualifier = QUAL_HH;
                fmt++;
            }
        } else if (*fmt == 'l') {
            fmt++;
            qualifier = QUAL_L;
            if (*fmt == 'l') {
                qualifier = QUAL_LL;
                fmt++;
            }
        } else if (*fmt == 'z' || *fmt == 'Z') {
            qualifier = QUAL_Z;
            fmt++;
        } else if (*fmt == 't') {
            qualifier = QUAL_T;
            fmt++;
        }

        conversion = *fmt;
        if (!conversion) {
            output_char(&out, '%');
            break;
        }
        fmt++;

        switch (conversion) {
        case '%':
            output_char(&out, '%');
            break;
        case 'c': {
            char c = (char)va_arg(ap, int);
            int padding = width > 1 ? width - 1 : 0;

            if (!(flags & FORMAT_LEFT))
                output_repeat(&out, ' ', padding);
            output_char(&out, c);
            if (flags & FORMAT_LEFT)
                output_repeat(&out, ' ', padding);
            break;
        }
        case 's':
            format_string(&out, va_arg(ap, const char *), flags,
                          width, precision);
            break;
        case 'p': {
            unsigned long long value =
                (unsigned long long)(uintptr_t)va_arg(ap, void *);

            flags |= FORMAT_SPECIAL | FORMAT_ZEROPAD | FORMAT_PREFIX_ZERO;
            if (width < 0)
                width = (int)(2 + sizeof(void *) * 2);
            format_number(&out, value, 0, 16, 0, flags, width, -1);
            break;
        }
        case 'd':
        case 'i': {
            long long signed_value = get_signed_arg(&ap, qualifier);
            unsigned long long magnitude;
            int negative = signed_value < 0;

            if (negative)
                magnitude = (unsigned long long)(-(signed_value + 1)) + 1;
            else
                magnitude = (unsigned long long)signed_value;
            format_number(&out, magnitude, negative, 10, 0, flags,
                          width, precision);
            break;
        }
        case 'u':
            format_number(&out, get_unsigned_arg(&ap, qualifier), 0, 10,
                          0, flags, width, precision);
            break;
        case 'o':
            format_number(&out, get_unsigned_arg(&ap, qualifier), 0, 8,
                          0, flags, width, precision);
            break;
        case 'x':
            format_number(&out, get_unsigned_arg(&ap, qualifier), 0, 16,
                          0, flags, width, precision);
            break;
        case 'X':
            format_number(&out, get_unsigned_arg(&ap, qualifier), 0, 16,
                          1, flags, width, precision);
            break;
        case 'b':
            format_number(&out, get_unsigned_arg(&ap, qualifier), 0, 2,
                          0, flags, width, precision);
            break;
        default:
            /* Keep unsupported conversions visible without consuming args. */
            output_char(&out, '%');
            output_char(&out, conversion);
            break;
        }
    }
    va_end(ap);

    if (out.size) {
        size_t terminator = out.count < out.size ? out.count : out.size - 1;
        out.buf[terminator] = '\0';
    }

    return out.count > INT_MAX ? INT_MAX : (int)out.count;
}

int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    int wanted = vsnprintf(buf, size, fmt, args);

    if (!size)
        return 0;
    if ((size_t)wanted >= size)
        return size - 1 > INT_MAX ? INT_MAX : (int)(size - 1);
    return wanted;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return length;
}

int scnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = vscnprintf(buf, size, fmt, args);
    va_end(args);
    return length;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    return vsnprintf(buf, (size_t)-1, fmt, args);
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = vsnprintf(buf, (size_t)-1, fmt, args);
    va_end(args);
    return length;
}
