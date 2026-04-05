/**
 * @FilePath     : /ZZZ-OS/arch/arm/include/asm/spinlock.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-11 01:59:31
 * @LastEditTime : 2026-03-11 02:01:18
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
 #ifndef ARM_SPINLOCK_H
 #define ARM_SPINLOCK_H
 
 #include <os/types.h>

 /* ARMv7无RISC-V的AMO原子交换指令，需用LDREX/STREX实现原子操作 */
 typedef struct spinlock {
     volatile int lock;  // 锁状态：0=未持有，1=已持有
 } spinlock_t;
 
 #define SPINLOCK_INIT \
     { 0 }
 
 /**
  * @brief 初始化自旋锁
  * @param lock 自旋锁指针
  */
 static inline void spinlock_init(spinlock_t *lock) {
     lock->lock = 0;
 }
 
 static inline void spin_lock(spinlock_t *lock) {
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
            : "r"(&lock->lock), "r"(val)
            : "memory", "cc");
    } while (status != 0);

    asm volatile("dmb ish" ::: "memory");
}

static inline void spin_unlock(spinlock_t *lock) {
    asm volatile("dmb ish" ::: "memory");
    lock->lock = 0;
}
 
 #endif