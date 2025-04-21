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


#define PLIC_BASE                    0x0c000000
#define PLIC_PRIORITY_BASE            (PLIC_BASE + (0x0000))
#define PLIC_PENDING_BASE             (PLIC_BASE + (0x1000))
#define PLIC_INT_EN_BASE              (PLIC_BASE + (0x2000))
#define PLIC_INT_THRSHOLD_BASE        (PLIC_BASE + (0x200000))
#define PLIC_CLAIM_BASE               (PLIC_BASE + (0x200004))
#define PLIC_COMPLETE_BASE            (PLIC_BASE + (0x200004))

#define UART0_IRQN 10

/***************************************************************
 * @description: 
 * @param {uint32_t} irqn [in/out]:  
 * @param {uint32_t} priority [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void __plic_priority_set(uint32_t irqn,uint32_t priority)
{
    volatile uint32_t *plic_priority = (volatile uint32_t *)PLIC_PRIORITY_BASE;
    plic_priority[irqn] = priority;  // 直接数组访问，编译器自动计算偏移
}

/***************************************************************
 * @description: 
 * @param {uint32_t} irqn [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE uint32_t __plic_priority_get(uint32_t irqn)
{
    volatile uint32_t *plic_priority = (volatile uint32_t *)PLIC_PRIORITY_BASE;
    return plic_priority[irqn];
}

/***************************************************************
 * @description: 
 * @param {uint32_t} irqn [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE uint32_t __plic_pending_get(uint32_t irqn)
{
    volatile uint32_t *plic_pending = (volatile uint32_t *)PLIC_PENDING_BASE;
    return plic_pending[irqn/32] & (1<<(irqn%32)) ?1:0;
}

/***************************************************************
 * @description: 
 * @param {uint32_t} hart [in/out]:  
 * @param {uint32_t} irqn [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void __plic_interrupt_enable(uint32_t hart,uint32_t irqn)
{
    volatile uint32_t *plic_int_en = (volatile uint32_t *)PLIC_INT_EN_BASE;
    plic_int_en[hart*0x80 + 4*(irqn/32)] |= (1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {uint32_t} hart [in/out]:  
 * @param {uint32_t} irqn [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void __plic_interrupt_disable(uint32_t hart,uint32_t irqn)
{
    volatile uint32_t *plic_int_en = (volatile uint32_t *)PLIC_INT_EN_BASE;
    plic_int_en[hart*0x80 + 4*(irqn/32)]  &= ~(1<<(irqn%32));
}

/***************************************************************
 * @description: 
 * @param {uint32_t} hart [in/out]:  
 * @param {uint32_t} threshold [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void __plic_threshold_set(uint32_t hart,uint32_t threshold)
{
    volatile uint32_t *plic_int_thrshold = (volatile uint32_t *)PLIC_INT_THRSHOLD_BASE;
    plic_int_thrshold[hart*0x1000] = threshold;
}

/***************************************************************
 * @description: 
 * @param {uint32_t} hart [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE uint32_t __plic_claim(uint32_t hart)
{
    volatile uint32_t *plic_claim = (volatile uint32_t *)PLIC_CLAIM_BASE ;
    return  plic_claim[hart*0x1000];
}

/***************************************************************
 * @description: 
 * @param {uint32_t} hart [in/out]:  
 * @param {uint32_t} irqn [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void __plic_complete(uint32_t hart,uint32_t irqn)
{
    volatile uint32_t *plic_claim = (volatile uint32_t *)PLIC_CLAIM_BASE ;
    plic_claim[hart*0x1000]= irqn;
}


#endif