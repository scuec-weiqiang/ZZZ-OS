#ifndef __RISCV64_UACCESS_H
#define __RISCV64_UACCESS_H

#include <os/types.h>

#define EFAULT 14
#define USER_LIMIT 0x0000004000000000UL

static inline int __range_ok(unsigned long addr, size_t size)
{
    unsigned long end;

    if (size > USER_LIMIT) {
        return 1;
    }

    end = addr + size;
    if (end < addr) {
        return 1;
    }

    return end > USER_LIMIT;
}

#define access_ok(addr, size) (__range_ok((unsigned long)(addr), size) == 0)

#define __put_user(x, ptr)                                                  \
({                                                                          \
    int __err = 0;                                                          \
    __typeof__(*(ptr)) __val = (x);                                         \
    unsigned long __addr = (unsigned long)(ptr);                            \
                                                                            \
    if (__range_ok(__addr, sizeof(*(ptr)))) {                               \
        __err = -EFAULT;                                                    \
    } else {                                                                \
        *(volatile __typeof__(*(ptr)) *)__addr = __val;                     \
    }                                                                       \
    __err;                                                                  \
})

#define __get_user(x, ptr)                                                  \
({                                                                          \
    int __err = 0;                                                          \
    unsigned long __addr = (unsigned long)(ptr);                            \
                                                                            \
    if (__range_ok(__addr, sizeof(x))) {                                    \
        __err = -EFAULT;                                                    \
        (x) = 0;                                                            \
    } else {                                                                \
        (x) = *(volatile __typeof__(x) *)__addr;                            \
    }                                                                       \
    __err;                                                                  \
})

static inline long __copy_to_user(void __user *to, const void *from, size_t len)
{
    size_t i;

    if (__range_ok((unsigned long)to, len)) {
        return -EFAULT;
    }

    for (i = 0; i < len; i++) {
        ((volatile u8 *)to)[i] = ((const u8 *)from)[i];
    }

    return 0;
}

static inline long __copy_from_user(void *to, const void __user *from, size_t len)
{
    size_t i;

    if (__range_ok((unsigned long)from, len)) {
        return -EFAULT;
    }

    for (i = 0; i < len; i++) {
        ((u8 *)to)[i] = ((const volatile u8 *)from)[i];
    }

    return 0;
}

#endif
