#ifndef __ASM_BITOPS_H
#define __ASM_BITOPS_H

#include <os/types.h>

static inline int constant_fls(unsigned long x)
{
    int r;

    if (!x) {
        return 0;
    }

    r = 0;
    while (x != 0) {
        x >>= 1;
        r++;
    }

    return r;
}

static inline int fls(int x)
{
    if (__builtin_constant_p(x)) {
        return constant_fls((unsigned long)x);
    }
    if (x == 0) {
        return 0;
    }

    return (int)(sizeof(unsigned int) * 8 - __builtin_clz((unsigned int)x));
}

static inline unsigned long __fls(unsigned long x)
{
    if (sizeof(unsigned long) == 8) {
        return (unsigned long)(63 - __builtin_clzl(x));
    }
    return (unsigned long)(31 - __builtin_clz((unsigned int)x));
}

static inline int ffs(int x)
{
    if (x == 0) {
        return 0;
    }
    return __builtin_ffs(x);
}

static inline unsigned long __ffs(unsigned long x)
{
    return (unsigned long)(__builtin_ffsl((long)x) - 1);
}

#define ffz(x) __ffs(~(x))

#endif /* __ASM_BITOPS_H */
