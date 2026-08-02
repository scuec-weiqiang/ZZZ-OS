#ifndef __ASM_ATOMIC_H
#define __ASM_ATOMIC_H

#include <os/types.h>
#include <stdint.h>
#include <asm-generic/atomic.h>

#define ATOMIC_INIT(i)	{ (i) }

// ------------------------------
// 32位 原子操作 (int)
// ------------------------------

static inline int atomic_read(const atomic_t *v)
{
    return __atomic_load_n(&v->counter, __ATOMIC_RELAXED);
}

static inline void atomic_set(atomic_t *v, int i)
{
    __atomic_store_n(&v->counter, i, __ATOMIC_RELAXED);
}

static inline void atomic_add(int i, atomic_t *v)
{
    (void)__atomic_fetch_add(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline void atomic_sub(int i, atomic_t *v)
{
    (void)__atomic_fetch_sub(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline void atomic_inc(atomic_t *v)
{
    atomic_add(1, v);
}

static inline void atomic_dec(atomic_t *v)
{
    atomic_sub(1, v);
}

static inline int atomic_add_return(int i, atomic_t *v)
{
    return __atomic_add_fetch(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
    return __atomic_sub_fetch(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline int atomic_inc_return(atomic_t *v)
{
    return atomic_add_return(1, v);
}

static inline int atomic_dec_return(atomic_t *v)
{
    return atomic_sub_return(1, v);
}

static inline int atomic_xchg(atomic_t *v, int new_val)
{
    return __atomic_exchange_n(&v->counter, new_val, __ATOMIC_ACQ_REL);
}

static inline int atomic_cmpxchg(atomic_t *v, int old_val, int new_val)
{
    return __atomic_compare_exchange_n(
        &v->counter,
        &old_val,
        new_val,
        0,
        __ATOMIC_ACQ_REL,
        __ATOMIC_RELAXED
    );
}

#if SYS_BITS == 64

static inline long atomic64_read(const atomic64_t *v)
{
    return __atomic_load_n(&v->counter, __ATOMIC_RELAXED);
}

static inline void atomic64_set(atomic64_t *v, long i)
{
    __atomic_store_n(&v->counter, i, __ATOMIC_RELAXED);
}

static inline void atomic64_add(long i, atomic64_t *v)
{
    (void)__atomic_fetch_add(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline void atomic64_sub(long i, atomic64_t *v)
{
    (void)__atomic_fetch_sub(&v->counter, i, __ATOMIC_ACQ_REL);
}

static inline void atomic64_inc(atomic64_t *v)
{
    atomic64_add(1, v);
}

static inline void atomic64_dec(atomic64_t *v)
{
    atomic64_sub(1, v);
}

// 64位 CAS
static inline long atomic64_cmpxchg(atomic64_t *v, long old_val, long new_val)
{
    return __atomic_compare_exchange_n(
        &v->counter,
        &old_val,
        new_val,
        0,
        __ATOMIC_ACQ_REL,
        __ATOMIC_RELAXED
    );
}
#endif

#endif
