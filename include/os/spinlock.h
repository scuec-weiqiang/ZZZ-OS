#ifndef _OS_SPINLOCK_H
#define _OS_SPINLOCK_H

#include <asm/spinlock.h>
#include <asm/irq.h>
#include <os/preempt.h>

typedef struct spinlock {
    arch_spinlock_t raw_lock;
} spinlock_t;

#define SPINLOCK_INIT { .raw_lock = ARCH_SPINLOCK_INIT }

static inline void spin_lock_init(spinlock_t *lock)
{
    lock->raw_lock = (arch_spinlock_t)ARCH_SPINLOCK_INIT;
}

static inline void spin_lock(spinlock_t *lock)
{
    arch_spin_lock(&lock->raw_lock);
}

static inline void spin_unlock(spinlock_t *lock)
{
    arch_spin_unlock(&lock->raw_lock);
}

static inline int spin_trylock(spinlock_t *lock)
{
    return arch_spin_trylock(&lock->raw_lock);
}

static inline unsigned long spin_lock_irqsave(spinlock_t *lock)
{
    unsigned long flags = arch_local_irq_save();
    arch_spin_lock(&lock->raw_lock);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
    arch_spin_unlock(&lock->raw_lock);
    arch_local_irq_restore(flags);
}

#endif