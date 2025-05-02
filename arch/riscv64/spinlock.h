/**
 * @FilePath: /ZZZ/arch/riscv64/spinlock.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-01 23:29:09
 * @LastEditTime: 2025-05-02 00:24:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
// spinlock.h
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "types.h"

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

#define SPINLOCK_INIT {0}

/**
 * @brief 自旋锁函数
 *
 * 尝试获取自旋锁。如果锁已经被其他线程持有，则当前线程将自旋等待直到锁被释放。
 *
 * @param lock 自旋锁指针
 */
__SELF __INLINE void spin_lock(spinlock_t *lock) {
    uint32_t value = 1;
    do{
        asm volatile (
            "amoswap.w.aq %0, %1, (%2)"
            : "=r"(value)
            : "r"(value), "r"(&lock->lock)
            : "memory"
        );
    }while(value != 0);
}

/**
 * @brief 解除自旋锁
 *
 * 使用原子操作解除自旋锁，确保多线程环境下的线程安全。
 *
 * @param lock 指向自旋锁对象的指针
 */
__SELF __INLINE void spin_unlock(spinlock_t *lock) {
    asm volatile (
        "amoswap.w.rl zero,zero,(%0)"
        :
        : "r"(&lock->lock)
        : "memory"
    );
}

#endif