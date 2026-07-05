#ifndef RISCV_SPINLOCK_H
#define RISCV_SPINLOCK_H

#include <asm-generic/spinlock_types.h>

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
    int old;
    int val = 1;

    do {
        __asm volatile(
            "amoswap.w.aq %0, %1, (%2)"
            : "=r"(old)
            : "r"(val), "r"(&lock->val)
            : "memory");
        if (old != 0) {
            __asm volatile("nop" ::: "memory");
        }
    } while (old != 0);
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
    __asm volatile(
        "amoswap.w.rl zero, zero, (%0)"
        :
        : "r"(&lock->val)
        : "memory");
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
    int old;
    int val = 1;

    __asm volatile(
        "amoswap.w.aq %0, %1, (%2)"
        : "=r"(old)
        : "r"(val), "r"(&lock->val)
        : "memory");

    return old == 0;
}

#endif
