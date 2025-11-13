/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-11-12 23:18:55
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-27 23:20:09
 * @FilePath: /my_code/source/interrupt.c
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <os/types.h>
#include <asm/riscv.h>
#include <asm/platform.h>
// #include <asm/riscv_plic.h>

#define CLINT_IRQ_SOFT 1
#define CLINT_IRQ_TIMER 5 
#define CLINT_IRQ_EXTERN 9


static inline void m_global_interrupt_enable()
{
    mstatus_w(mstatus_r()|(0x08));
}

static inline void m_global_interrupt_disable()
{
    mstatus_w(mstatus_r()&(~0x08));
}



static inline void m_timer_interrupt_enable()
{
    mie_w(mie_r()|0x80);
}

static inline void m_timer_interrupt_disable()
{
    mie_w(mie_r()&(~0x80));
}



static inline void m_extern_interrupt_enable()
{
    mie_w(mie_r()|0x800);
}

static inline void m_extern_interrupt_disable()
{
    mie_w(mie_r()&(~0x800));
}

static inline void m_soft_interrupt_enable()
{
    mie_w(mie_r()|0x08);
}
static inline void m_soft_interrupt_disable()
{
    mie_w(mie_r()&(~0x08));
}



static inline void s_global_interrupt_enable()
{
    sstatus_w(sstatus_r()|(0x02));
}

static inline void s_global_interrupt_disable()
{
    sstatus_w(sstatus_r()&(~0x02));
}


// 定时器中断
static inline void s_timer_interrupt_enable()
{
    sie_w(sie_r()|0x20);
}

static inline void s_timer_interrupt_disable()
{
    sie_w(sie_r()&(~0x20));
}

static inline void s_timer_interrupt_pending()
{
    sip_w(sip_r()|0x20);
}

static inline void s_timer_interrupt_clear_pending()
{
    sip_w(sip_r()&(~0x20));
}

static inline int s_timer_interrupt_get_pending()
{
    return (sip_r()&0x20)?1:0;
}


// 外部中断
static inline void s_extern_interrupt_enable()
{
    sie_w(sie_r()|(1<<9));
}

static inline void s_extern_interrupt_disable()
{
    sie_w(sie_r()&(~(1<<9)));
}

static inline void s_extern_interrupt_pending()
{
    sip_w(sip_r()|(1<<9));
}

static inline void s_extern_interrupt_clear_pending()
{
    sip_w(sip_r()&(~(1<<9)));
}

static inline int s_extern_interrupt_get_pending()
{
    return (sip_r()&(1<<9))?1:0;
}

// static inline void extern_interrupt_setting(enum hart_id hart_id,uint32_t iqrn,uint32_t priority)
// { 
//     __plic_priority_set(iqrn,priority);
//     __plic_threshold_set(hart_id,0);
//     __plic_interrupt_enable(hart_id,iqrn);
// } 



// 软件中断
static inline void s_soft_interrupt_enable()
{
    sie_w(sie_r()|0x02);
}

static inline void s_soft_interrupt_disable()
{
    sie_w(sie_r()&(~0x02));
}

static inline void s_soft_interrupt_pending()
{
    sip_w(sip_r()|0x02);
}

static inline void s_soft_interrupt_clear_pending()
{
    sip_w(sip_r()&(~0x02));
}

static inline int s_soft_interrupt_get_pending()
{
    return (sip_r()&0x02)?1:0;
}

#endif