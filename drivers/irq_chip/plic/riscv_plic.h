/*******************************************************************************************
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/asm/plic.h
 * @Description  : 平台中断控制器头文件，用于屏蔽中断和设置中断优先级等操作。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 15:03:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#ifndef PLIC_H
#define PLIC_H

#include <os/types.h>
extern virt_addr_t plic_base;

#define PLIC_BASE                     (plic_base)  // PLIC 基地址，由驱动程序初始化时映射
#define PLIC_PRIORITY_BASE            (PLIC_BASE + (0x0000))
#define PLIC_PENDING_BASE             (PLIC_BASE + (0x1000))
#define PLIC_INT_EN_BASE              (PLIC_BASE + (0x2000))
#define PLIC_INT_THRSHOLD_BASE        (PLIC_BASE + (0x200000))
#define PLIC_CLAIM_BASE               (PLIC_BASE + (0x200004))
#define PLIC_COMPLETE_BASE            (PLIC_BASE + (0x200004))


static inline u32 plic_get_context(u32 cpu)
{
    // unsigned long sstatus;
    // asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    // // 如果sstatus可访问且SIE位可读写，说明当前在S态
    // return (sstatus & 0x2) ? (cpu * 2 + 1) : (cpu * 2);
    return cpu * 2 + 1; // 仅支持S态
}

/***************************************************************
 * @description: 
 * @param {u32} irqn [in/out]:  
 * @param {u32} priority [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_priority_set(u32 irqn,u32 priority)
{
    volatile u32 *plic_priority = (volatile u32 *)PLIC_PRIORITY_BASE;
    plic_priority[irqn] = priority;  // 直接数组访问，编译器自动计算偏移
}

/***************************************************************
 * @description: 
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline u32 __plic_priority_get(u32 irqn)
{
    volatile u32 *plic_priority = (volatile u32 *)PLIC_PRIORITY_BASE;
    return plic_priority[irqn];
}

/***************************************************************
 * @description: 
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline u32 __plic_pending_get(u32 irqn)
{
    volatile u32 *plic_pending = (volatile u32 *)PLIC_PENDING_BASE;
    return plic_pending[irqn/32] & (1<<(irqn%32)) ?1:0;
}

/***************************************************************
 * @description: 
 * @param {u32} cpu [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static inline void __plic_interrupt_enable(u32 cpu,u32 irqn)
{
    volatile u32 *plic_int_en = (volatile u32 *)PLIC_INT_EN_BASE;
    plic_int_en[plic_get_context(cpu) * 0x80/4 + 4*(irqn/32)] |= (1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {u32} cpu [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_interrupt_disable(u32 cpu,u32 irqn)
{
    volatile u32 *plic_int_en = (volatile u32 *)PLIC_INT_EN_BASE;
    plic_int_en[plic_get_context(cpu) * 0x80/4 + 4*(irqn/32)]  &= ~(1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {u32} cpu [in/out]:  
 * @param {u32} threshold [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_threshold_set(u32 cpu,u32 threshold)
{
    volatile u32 *plic_int_thrshold = (volatile u32 *)PLIC_INT_THRSHOLD_BASE;
    plic_int_thrshold[plic_get_context(cpu) * 0x1000] = threshold;
}

/***************************************************************
 * @description: 
 * @param {u32} cpu [in/out]:  
 * @return {*}
***************************************************************/
static  inline u32 __plic_claim(u32 cpu)
{
    volatile u32 *plic_claim = (volatile u32 *)PLIC_CLAIM_BASE ;
    return  plic_claim[plic_get_context(cpu) * 0x1000/4];
}

/***************************************************************
 * @description: 
 * @param {u32} cpu [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_complete(u32 cpu,u32 irqn)
{
    volatile u32 *plic_complete = (volatile u32 *)PLIC_COMPLETE_BASE;
    plic_complete[plic_get_context(cpu) * 0x1000/4]= irqn;
}


#endif