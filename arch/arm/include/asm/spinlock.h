#ifndef _ARM_SPINLOCK_H
#define _ARM_SPINLOCK_H

#include <os/types.h>
#include <asm-generic/spinlock_types.h>

static inline void arch_spin_lock(arch_spinlock_t *lock) {
    int old, status;
    const int val = 1;

    do {
        asm volatile(
            "ldrex   %0, [%2]\n"
            "cmp     %0, #0\n"
            "bne     1f\n"
            "strex   %1, %3, [%2]\n"
            "b       2f\n"
            "1:\n"
            "clrex\n"
            "mov     %1, #1\n"
            "2:\n"
            : "=&r"(old), "=&r"(status)
            : "r"(&lock->val), "r"(val)
            : "memory", "cc");
    } while (status != 0);

    asm volatile("dmb ish" ::: "memory");
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
    asm volatile("dmb ish" ::: "memory");
    WRITE_ONCE(lock->val, 0);
}

static inline int arch_spin_trylock(arch_spinlock_t *lock) {
    int old, status;
    const int val = 1;

    asm volatile(
        "ldrex   %0, [%2]\n"
        "cmp     %0, #0\n"
        "bne     1f\n"
        "strex   %1, %3, [%2]\n"
        "b       2f\n"
        "1:\n"
        "clrex\n"
        "mov     %1, #1\n"
        "2:\n"
        : "=&r"(old), "=&r"(status)
        : "r"(&lock->val), "r"(val)
        : "memory", "cc");

    if (status == 0)
        asm volatile("dmb ish" ::: "memory");

    return status == 0;
}

#endif