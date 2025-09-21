/*******************************************************************************************
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/plic.h
 * @Description  : 平台中断控制器头文件，用于屏蔽中断和设置中断优先级等操作。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 15:03:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#ifndef PLIC_H
#define PLIC_H

#include "types.h"
#include "platform.h"

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
 * @param {u32} hart [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static inline void __plic_interrupt_enable(u32 hart,u32 irqn)
{
    volatile u32 *plic_int_en = (volatile u32 *)PLIC_INT_EN_BASE;
    plic_int_en[hart*0x80 + 4*(irqn/32)] |= (1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {u32} hart [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_interrupt_disable(u32 hart,u32 irqn)
{
    volatile u32 *plic_int_en = (volatile u32 *)PLIC_INT_EN_BASE;
    plic_int_en[hart*0x80 + 4*(irqn/32)]  &= ~(1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {u32} hart [in/out]:  
 * @param {u32} threshold [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_threshold_set(u32 hart,u32 threshold)
{
    volatile u32 *plic_int_thrshold = (volatile u32 *)PLIC_INT_THRSHOLD_BASE;
    plic_int_thrshold[hart*0x1000] = threshold;
}

/***************************************************************
 * @description: 
 * @param {u32} hart [in/out]:  
 * @return {*}
***************************************************************/
static  inline u32 __plic_claim(u32 hart)
{
    volatile u32 *plic_claim = (volatile u32 *)PLIC_CLAIM_BASE ;
    return  plic_claim[hart*0x1000];
}

/***************************************************************
 * @description: 
 * @param {u32} hart [in/out]:  
 * @param {u32} irqn [in/out]:  
 * @return {*}
***************************************************************/
static  inline void __plic_complete(u32 hart,u32 irqn)
{
    volatile u32 *plic_claim = (volatile u32 *)PLIC_CLAIM_BASE ;
    plic_claim[hart*0x1000]= irqn;
}


#endif