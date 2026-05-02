#ifndef __ARM_UACCESS_H
#define __ARM_UACCESS_H

#include <os/types.h>


#define EFAULT        14
#define USER_LIMIT    0xC0000000  // 用户空间上限


static inline int __range_ok(unsigned long addr, size_t size)
{
    unsigned long flag, end;
    unsigned long limit = USER_LIMIT;

    __asm__ __volatile__(
        "adds   %1, %2, %3\n"
        "sbcccs %1, %1, %0\n"
        "movcc  %0, #0\n"
        : "=&r"(flag), "=&r"(end)
        : "r"(addr), "r"(size), "0"(limit)
        : "cc"
    );
    return flag;
}

#define access_ok(addr, size)  (__range_ok((unsigned long)(addr), size))


#define __put_user(x, ptr)                            \
({                                                    \
    int __err = 0;                                    \
    __typeof__(*(ptr)) __val = (x);                       \
    unsigned long __addr = (unsigned long)(ptr);      \
    size_t __size = sizeof(*(ptr));                   \
                                                      \
    if (__range_ok(__addr, __size)) {                 \
        __err = -EFAULT;                              \
    } else {                                          \
		switch (__size) {                            \
			case 1:                                   \
				__asm__ __volatile__(                 \
					"strb %0, [%1]\n"                 \
					: : "r"(__val), "r"(__addr)       \
					: "memory");                        \
					break;                              \
			case 2:                                   \
				__asm__ __volatile__(                 \
					"strh %0, [%1]\n"                 \
					: : "r"(__val), "r"(__addr)       \
					: "memory");                       \
					break;                              \
        	case 4 :                                   \
				__asm__ __volatile__(                     \
					"str  %0, [%1]\n"                     \
					: : "r"(__val), "r"(__addr)           \
					: "memory");                          \
					break;                              \
			default:\
				__err = -EFAULT;                              \
				break;                              \
		}\
    }                                                 \
    __err;                                            \
})

#define __get_user(x, ptr)                            \
({                                                    \
    int __err = 0;                                    \
    unsigned long __addr = (unsigned long)(ptr);      \
    size_t __size = sizeof(x);                        \
                                                      \
    if (__range_ok(__addr, __size)) {                 \
        __err = -EFAULT;                              \
        (x) = 0;                                      \
    } else {                                          \
		switch (__size) {                            \
			case 1:                                   \
            __asm__ __volatile__(                     \
                "ldrb %0, [%1]\n"                     \
                : "=r"(x)                             \
                : "r"(__addr)                         \
                : "memory");break;                          \
        case 2:\
            __asm__ __volatile__(                     \
                "ldrh %0, [%1]\n"                     \
                : "=r"(x)                             \
                : "r"(__addr)                         \
                : "memory");break;                           \
        case 4:                    \
            __asm__ __volatile__(                     \
                "ldr  %0, [%1]\n"                     \
                : "=r"(x)                             \
                : "r"(__addr)                         \
                : "memory");break;                           \
		default:\
			__err = -EFAULT;                              \
			(x) = 0;                                      \
			break;                              \
        }                                               \
    }                                                 \
    __err;                                            \
})


static inline long __copy_to_user(void __user *to,
                                const void *from, size_t len)
{
    if (__range_ok((unsigned long)to, len))
        return -EFAULT;

    __asm__ __volatile__(
        "mov r2, #0\n"
        "0: ldrb r3, [%1], #1\n"
        "   strb r3, [%0], #1\n"
        "   add  r2, r2, #1\n"
        "   cmp  r2, %2\n"
        "   bne  0b\n"
        : "+r"(to), "+r"(from)
        : "r"(len)
        : "memory", "r2", "r3"
    );
    return 0;
}

static inline long __copy_from_user(void *to,
                                 const void __user *from, size_t len)
{
    if (__range_ok((unsigned long)from, len))
        return -EFAULT;

    __asm__ __volatile__(
        "mov r2, #0\n"
        "0: ldrb r3, [%1], #1\n"
        "   strb r3, [%0], #1\n"
        "   add  r2, r2, #1\n"
        "   cmp  r2, %2\n"
        "   bne  0b\n"
        : "+r"(to), "+r"(from)
        : "r"(len)
        : "memory", "r2", "r3"
    );
    return 0;
}





#endif /* __ARM_UACCESS_H */