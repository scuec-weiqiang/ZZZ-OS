#include <stdarg.h>
#include <os/types.h>
#include <usys.h>

#define PRINT_BUF_SIZE 1024

static char print_buf[PRINT_BUF_SIZE];

static void buf_putc(char *buf, int *pos, char ch)
{
    if (*pos < PRINT_BUF_SIZE - 1) {
        buf[*pos] = ch;
    }
    (*pos)++;
}

static void buf_puts(char *buf, int *pos, const char *s)
{
    if (s == NULL) {
        s = "(null)";
    }

    while (*s) {
        buf_putc(buf, pos, *s);
        s++;
    }
}

static void buf_put_uint(char *buf, int *pos, unsigned int value, unsigned int base)
{
    char tmp[16];
    int i = 0;

    if (base < 2 || base > 16) {
        return;
    }

    if (value == 0) {
        buf_putc(buf, pos, '0');
        return;
    }

    while (value != 0) {
        unsigned int digit = value % base;
        if (digit < 10) {
            tmp[i++] = (char)('0' + digit);
        } else {
            tmp[i++] = (char)('a' + digit - 10);
        }
        value /= base;
    }

    while (i > 0) {
        i--;
        buf_putc(buf, pos, tmp[i]);
    }
}

static void buf_put_int(char *buf, int *pos, int value)
{
    unsigned int mag;

    if (value < 0) {
        buf_putc(buf, pos, '-');
        mag = (unsigned int)(-(value + 1)) + 1;
    } else {
        mag = (unsigned int)value;
    }

    buf_put_uint(buf, pos, mag, 10);
}

static int vsnprintf_simple(char *buf, const char *fmt, va_list ap)
{
    int pos = 0;

    while (*fmt) {
        if (*fmt != '%') {
            buf_putc(buf, &pos, *fmt);
            fmt++;
            continue;
        }

        fmt++;

        if (*fmt == '%') {
            buf_putc(buf, &pos, '%');
            fmt++;
            continue;
        }

        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 't') {
            fmt++;
        }

        switch (*fmt) {
        case 'd':
        case 'i':
            buf_put_int(buf, &pos, va_arg(ap, int));
            break;
        case 'u':
            buf_put_uint(buf, &pos, va_arg(ap, unsigned int), 10);
            break;
        case 'x':
        case 'X':
            buf_put_uint(buf, &pos, va_arg(ap, unsigned int), 16);
            break;
        case 'p':
            buf_puts(buf, &pos, "0x");
            buf_put_uint(buf, &pos, (unsigned int)(uintptr_t)va_arg(ap, void *), 16);
            break;
        case 'c':
            buf_putc(buf, &pos, (char)va_arg(ap, int));
            break;
        case 's':
            buf_puts(buf, &pos, va_arg(ap, const char *));
            break;
        default:
            buf_putc(buf, &pos, '%');
            if (*fmt != '\0') {
                buf_putc(buf, &pos, *fmt);
            } else {
                pos--;
            }
            break;
        }

        if (*fmt != '\0') {
            fmt++;
        }
    }

    if (pos >= PRINT_BUF_SIZE) {
        buf[PRINT_BUF_SIZE - 1] = '\0';
    } else {
        buf[pos] = '\0';
    }

    return pos;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int len;
    int out_len;

    va_start(ap, fmt);
    len = vsnprintf_simple(print_buf, fmt, ap);
    va_end(ap);

    out_len = len;
    if (out_len >= PRINT_BUF_SIZE) {
        out_len = PRINT_BUF_SIZE - 1;
    }

    if (out_len > 0) {
        // write(1, print_buf, out_len);
        print(print_buf,out_len);
    }

    return len;
}

void panic(const char *fmt, ...)
{
    va_list ap;
    int len;
    int out_len;

    write(1, "panic: ", 7);

    va_start(ap, fmt);
    len = vsnprintf_simple(print_buf, fmt, ap);
    va_end(ap);

    out_len = len;
    if (out_len >= PRINT_BUF_SIZE) {
        out_len = PRINT_BUF_SIZE - 1;
    }

    if (out_len > 0) {
        write(1, print_buf, out_len);
    }
    write(1, "\n", 1);

    for (;;) {
    }
}
