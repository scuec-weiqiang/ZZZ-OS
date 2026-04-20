#ifndef __ASM_ATOMIC_H
#define __ASM_ATOMIC_H

#include <os/types.h>

#include <stdint.h>

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