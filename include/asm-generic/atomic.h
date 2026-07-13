#ifndef __ASM_GENERIC_ATOMIC_H
#define __ASM_GENERIC_ATOMIC_H

#define try_cmpxchg(ptr, oldp, newval)                                      \
({                                                                          \
    typeof(*(ptr)) __old = *(oldp);                                          \
    __atomic_compare_exchange_n((ptr), oldp, (newval), false,                \
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);         \
})

#define xchg(ptr, newval)                                                   \
({                                                                          \
    typeof(*(ptr)) __new = (newval);                                         \
    __atomic_exchange_n((ptr), __new, __ATOMIC_SEQ_CST);                     \
})

#define smp_load_acquire(ptr)                                                \
    __atomic_load_n((ptr), __ATOMIC_ACQUIRE)

#define smp_store_release(ptr, val)                                          \
    __atomic_store_n((ptr), (val), __ATOMIC_RELEASE)

#endif
