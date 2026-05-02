#include <os/types.h>
#include <stdarg.h>
#include <os/console.h>
#include <os/spinlock.h>
#include <os/utils.h>

#define PRINTK_BUF_SIZE (4096U * 4U)

static char printk_buf[PRINTK_BUF_SIZE] __attribute__((aligned(8)));
static char panic_buf[PRINTK_BUF_SIZE] __attribute__((aligned(8)));
static spinlock_t printk_lock = SPINLOCK_INIT;

#if SYS_BITS == 32
static unsigned int va_arg_u32_compat(va_list *ap)
{
    char *p;
    unsigned int v;

    __builtin_memcpy(&p, ap, sizeof(p));
    v = *(unsigned int *)p;
    p += sizeof(unsigned int);
    __builtin_memcpy(ap, &p, sizeof(p));

    return v;
}

static unsigned long long va_arg_u64_compat(va_list *ap)
{
    char *p;
    unsigned int lo;
    unsigned int hi;

    __builtin_memcpy(&p, ap, sizeof(p));
    p = (char *)ALIGN_UP((uintptr_t)p, sizeof(unsigned long long));
    lo = *(unsigned int *)p;
    hi = *(unsigned int *)(p + sizeof(unsigned int));
    p += sizeof(unsigned long long);
    __builtin_memcpy(ap, &p, sizeof(p));

    return ((unsigned long long)hi << 32) | lo;
}
#endif

static void buf_putc(char *buf, size_t size, size_t *pos, char c)
{
    if (size > 0 && *pos + 1 < size) {
        buf[*pos] = c;
    }
    (*pos)++;
}

static void buf_puts(char *buf, size_t size, size_t *pos, const char *s)
{
    const char *p = s ? s : "(null)";

    while (*p) {
        buf_putc(buf, size, pos, *p++);
    }
}

static void buf_putu(char *buf, size_t size, size_t *pos,
                     unsigned long long val, unsigned base,
                     int uppercase, int prefix)
{
    char tmp[64];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t n = 0;

    if (base < 2 || base > 16) {
        return;
    }

    if (prefix) {
        if (base == 16) {
            buf_putc(buf, size, pos, '0');
            buf_putc(buf, size, pos, uppercase ? 'X' : 'x');
        } else if (base == 2) {
            buf_putc(buf, size, pos, '0');
            buf_putc(buf, size, pos, 'b');
        }
    }

    if (val == 0) {
        buf_putc(buf, size, pos, '0');
        return;
    }

    while (val != 0 && n < sizeof(tmp)) {
        unsigned int rem = 0;
        val = divmod_u64(val, base, &rem);
        tmp[n++] = digits[rem];
    }

    while (n > 0) {
        buf_putc(buf, size, pos, tmp[--n]);
    }
}

static int kvsnprintk(char *buf, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;

    while (*fmt) {
        int long_cnt = 0;
        int z_mod = 0;
        char spec;

        if (*fmt != '%') {
            buf_putc(buf, size, &pos, *fmt++);
            continue;
        }

        fmt++;
        if (*fmt == '\0') {
            break;
        }

        if (*fmt == '%') {
            buf_putc(buf, size, &pos, '%');
            fmt++;
            continue;
        }

        while (*fmt == 'l') {
            long_cnt++;
            fmt++;
        }
        if (*fmt == 'z') {
            z_mod = 1;
            fmt++;
        }

        spec = *fmt;
        if (spec == '\0') {
            break;
        }
        fmt++;

        switch (spec) {
        case 'd':
        case 'i': {
            long long v;
            unsigned long long uv;

#if SYS_BITS == 32
            if (z_mod || long_cnt <= 1) {
                v = (long long)(int)va_arg_u32_compat(&ap);
            } else {
                v = (long long)va_arg_u64_compat(&ap);
            }
#else
            if (z_mod) {
                v = (long long)va_arg(ap, ssize_t);
            } else if (long_cnt >= 2) {
                v = va_arg(ap, long long);
            } else if (long_cnt == 1) {
                v = (long long)va_arg(ap, long);
            } else {
                v = (long long)va_arg(ap, int);
            }
#endif

            if (v < 0) {
                buf_putc(buf, size, &pos, '-');
                uv = (unsigned long long)(-(v + 1)) + 1ULL;
            } else {
                uv = (unsigned long long)v;
            }
            buf_putu(buf, size, &pos, uv, 10, 0, 0);
            break;
        }
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'b': {
            unsigned long long v;
            unsigned base = 10;
            int uppercase = 0;
            int prefix = 0;

#if SYS_BITS == 32
            if (z_mod || long_cnt <= 1) {
                v = (unsigned long long)va_arg_u32_compat(&ap);
            } else {
                v = va_arg_u64_compat(&ap);
            }
#else
            if (z_mod) {
                v = (unsigned long long)va_arg(ap, size_t);
            } else if (long_cnt >= 2) {
                v = va_arg(ap, unsigned long long);
            } else if (long_cnt == 1) {
                v = (unsigned long long)va_arg(ap, unsigned long);
            } else {
                v = (unsigned long long)va_arg(ap, unsigned int);
            }
#endif

            if (spec == 'x' || spec == 'X') {
                base = 16;
                uppercase = (spec == 'X');
            } else if (spec == 'o') {
                base = 8;
            } else if (spec == 'b') {
                base = 2;
                prefix = 1;
            }

            buf_putu(buf, size, &pos, v, base, uppercase, prefix);
            break;
        }
        case 'p': {
            uintptr_t v;

#if SYS_BITS == 32
            v = (uintptr_t)va_arg_u32_compat(&ap);
#else
            v = (uintptr_t)va_arg(ap, void *);
#endif
            buf_putu(buf, size, &pos, (unsigned long long)v, 16, 0, 1);
            break;
        }
        case 'c': {
            int ch;

#if SYS_BITS == 32
            ch = (int)va_arg_u32_compat(&ap);
#else
            ch = va_arg(ap, int);
#endif
            buf_putc(buf, size, &pos, (char)ch);
            break;
        }
        case 's': {
            const char *s;

#if SYS_BITS == 32
            s = (const char *)(uintptr_t)va_arg_u32_compat(&ap);
#else
            s = va_arg(ap, const char *);
#endif
            buf_puts(buf, size, &pos, s);
            break;
        }
        default:
            buf_putc(buf, size, &pos, '%');
            if (long_cnt == 1) {
                buf_putc(buf, size, &pos, 'l');
            } else if (long_cnt >= 2) {
                buf_putc(buf, size, &pos, 'l');
                buf_putc(buf, size, &pos, 'l');
            }
            if (z_mod) {
                buf_putc(buf, size, &pos, 'z');
            }
            buf_putc(buf, size, &pos, spec);
            break;
        }
    }

    if (size > 0) {
        size_t end = (pos < size - 1) ? pos : (size - 1);
        buf[end] = '\0';
    }

    return (int)pos;
}

int printk(const char *fmt, ...)
{
    va_list ap;
    unsigned long flags;
    int n;

    va_start(ap, fmt);
    n = kvsnprintk(printk_buf, sizeof(printk_buf), fmt, ap);
    va_end(ap);

    flags = spin_lock_irqsave(&printk_lock);
    // console_puts(printk_buf);
    // console_flush();
    extern void _puts(char *s) ;
    _puts(printk_buf);
    spin_unlock_irqrestore(&printk_lock, flags);

    return n;
}

void panic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    kvsnprintk(panic_buf, sizeof(panic_buf), fmt, ap);
    va_end(ap);

    console_puts("panic: ");
    console_puts(panic_buf);
    console_flush();

    while (1) {
    }
}
