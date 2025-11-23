/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/spinlock.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-01 16:50:04
 * @LastEditTime: 2025-11-21 16:26:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef RISCV_SPINLOCK_H
#define RISCV_SPINLOCK_H

typedef struct spinlock {
    volatile int lock;
} spinlock_t;

#define SPINLOCK_INIT \
    { 0 }

static inline void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}

/**
 * @brief 自旋锁函数
 *
 * 尝试获取自旋锁。如果锁已经被其他线程持有，则当前线程将自旋等待直到锁被释放。
 *
 * @param lock 自旋锁指针
 */
static inline void spin_lock(spinlock_t *lock) {
    int value = 1;
    do {
        asm volatile(
            "amoswap.w.aq %0, %1, (%2)"
            : "=r"(value)
            : "r"(value), "r"(&lock->lock)
            : "memory");
    } while (value != 0);
}

/**
 * @brief 解除自旋锁
 *
 * 使用原子操作解除自旋锁，确保多线程环境下的线程安全。
 *
 * @param lock 指向自旋锁对象的指针
 */
static inline void spin_unlock(spinlock_t *lock) {
    asm volatile(
        "amoswap.w.rl zero,zero,(%0)"
        :
        : "r"(&lock->lock)
        : "memory");
}

#endif