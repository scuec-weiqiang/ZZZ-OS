/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/interrupt.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 21:14:52
 * @LastEditTime: 2025-05-08 00:24:22
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-11-12 23:46:59
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-27 23:20:36
 * @FilePath: /my_code/include/interrupt.h
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "types.h"

extern void m_global_interrupt_enable();
extern void m_global_interrupt_disable();

extern void m_timer_interrupt_enable();
extern void m_timer_interrupt_disable();

extern void m_extern_interrupt_enable();
extern void m_extern_interrupt_disable();

extern void m_soft_interrupt_enable();
extern void m_soft_interrupt_disable();



extern void s_global_interrupt_enable();
extern void s_global_interrupt_disable();

extern void s_timer_interrupt_enable();
extern void s_timer_interrupt_disable();

extern void s_extern_interrupt_enable();
extern void s_extern_interrupt_disable();

extern void s_soft_interrupt_enable();
extern void s_soft_interrupt_disable();



extern void extern_interrupt_setting(uint64_t hart,uint64_t iqrn,uint64_t priority);

#endif
