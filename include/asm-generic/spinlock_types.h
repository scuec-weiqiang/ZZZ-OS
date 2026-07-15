#ifndef _ASM_GENERIC_SPINLOCK_TYPES_H
#define _ASM_GENERIC_SPINLOCK_TYPES_H

#include <asm-generic/rwonce.h>

typedef struct arch_spinlock {
    volatile int val;
} arch_spinlock_t;

#define ARCH_SPINLOCK_INIT { 0 }


#endif
