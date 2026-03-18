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
 
 /**
  * @brief 获取自旋锁（自旋等待）
  * @param lock 自旋锁指针
  * @note ARMv7使用LDREX/STREX实现原子交换，替代RISC-V的amoswap.w.aq
  */
 static inline void spin_lock(spinlock_t *lock) {
     int tmp;
     do {
         /* 
          * LDREX：加载并标记独占访问（对应RISC-V的.aq获取语义）
          * STREX：尝试独占存储，成功返回0，失败返回1
          * 逻辑：原子交换lock值为1，若原数值为0则获取成功
          */
         asm volatile(
             // 1. 独占加载lock的值到tmp
             "ldrex   %0, [%1]\n"
             // 2. 若加载的值不为0（锁已被持有），跳转到重试
             "cmp     %0, #0\n"
             "bne     1f\n"
             // 3. 尝试独占存储1到lock，结果存入tmp（0=成功，1=失败）
             "mov     %0, #1\n"
             "strex   %0, %0, [%1]\n"
             // 4. 检查存储结果，失败则重试
             "1:\n"
             : "=&r"(tmp)
             : "r"(&lock->lock)
             : "memory", "cc");  // 内存屏障 + 条件码寄存器
     } while (tmp != 0);  // tmp≠0表示未获取到锁，继续自旋
 }
 
 /**
  * @brief 释放自旋锁
  * @param lock 自旋锁指针
  * @note ARMv7使用DMB内存屏障保证操作顺序，对应RISC-V的.rl释放语义
  */
 static inline void spin_unlock(spinlock_t *lock) {
     asm volatile(
         // 1. 数据内存屏障（DMB）：保证之前的内存操作完成（释放语义）
         "dmb\n" 
         // 2. 直接写0释放锁（解锁无需原子交换，DMB已保证可见性）
         "mov     r0, #0\n"
         "str     r0, [%0]\n"
         :
         : "r"(&lock->lock)
         : "r0", "memory");
 }
 
 #endif