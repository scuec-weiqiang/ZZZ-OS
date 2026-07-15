#ifndef _OS_SPINLOCK_H
#define _OS_SPINLOCK_H

#include <asm/spinlock.h>
#include <asm/irq.h>
#include <os/cpu.h>
#include <os/preempt.h>

typedef struct spinlock {
    arch_spinlock_t raw_lock;
    int owner_cpu;
    void *owner_pc;
    int waiter_cpu;
    void *waiter_pc;
} spinlock_t;

#define SPINLOCK_INIT { \
    .raw_lock = ARCH_SPINLOCK_INIT, \
    .owner_cpu = -1, \
    .owner_pc = NULL, \
    .waiter_cpu = -1, \
    .waiter_pc = NULL, \
}

#define SPINLOCK_DEFINE(name) spinlock_t name = SPINLOCK_INIT

static inline void spin_lock_debug_wait(spinlock_t *lock, void *pc)
{
    lock->waiter_cpu = get_cpuid();
    lock->waiter_pc = pc;
}

static inline void spin_lock_debug_acquired(spinlock_t *lock, void *pc)
{
    lock->owner_cpu = get_cpuid();
    lock->owner_pc = pc;
    lock->waiter_cpu = -1;
    lock->waiter_pc = NULL;
}

static inline void spin_lock_debug_release(spinlock_t *lock)
{
    lock->owner_cpu = -1;
    lock->owner_pc = NULL;
}

static inline void spin_lock_init(spinlock_t *lock)
{
    lock->raw_lock = (arch_spinlock_t)ARCH_SPINLOCK_INIT;
    lock->owner_cpu = -1;
    lock->owner_pc = NULL;
    lock->waiter_cpu = -1;
    lock->waiter_pc = NULL;
}

static inline void spin_lock(spinlock_t *lock)
{
    void *pc = __builtin_return_address(0);

    preempt_disable();
    spin_lock_debug_wait(lock, pc);
    arch_spin_lock(&lock->raw_lock);
    spin_lock_debug_acquired(lock, pc);
}

static inline void spin_unlock(spinlock_t *lock)
{
    spin_lock_debug_release(lock);
    arch_spin_unlock(&lock->raw_lock);
    preempt_enable();
}

static inline int spin_trylock(spinlock_t *lock)
{
    void *pc = __builtin_return_address(0);
    int ret;

    ret = arch_spin_trylock(&lock->raw_lock);
    if (ret) {
        spin_lock_debug_acquired(lock, pc);
    }

    return ret;
}

static inline unsigned long spin_lock_irqsave(spinlock_t *lock)
{
    void *pc = __builtin_return_address(0);
    unsigned long flags = arch_local_irq_save();

    spin_lock_debug_wait(lock, pc);
    arch_spin_lock(&lock->raw_lock);
    spin_lock_debug_acquired(lock, pc);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
    spin_lock_debug_release(lock);
    arch_spin_unlock(&lock->raw_lock);
    arch_local_irq_restore(flags);
}

#endif
