/**
 * @FilePath: /ZZZ-OS/arch/riscv64/qemu_virt/systimer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-09-23 21:22:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "types.h"
#include "platform.h"
#include "interrupt.h"
#include "clint.h" 
#include "systimer.h"
#include "riscv.h"

//系统时钟以0核为基准
static u64 systimer_tick[MAX_HARTS_NUM] = {0};
static enum systimer_hz systimer_hz[MAX_HARTS_NUM] = {SYS_HZ_1,SYS_HZ_1};

inline void systimer_period(enum systimer_hz hz)
{
    enum hart_id id = tp_r();
    systimer_hz[id] = hz;
}

inline void systimer_reload()
{   
    enum hart_id id = tp_r();
    u64 temp = time_r();
    temp +=  systimer_hz[id];
    stimecmp_w(temp);
}

inline u64 systick()
{
    enum hart_id id = tp_r();
    return systimer_tick[id];
}

inline void systick_up()
{
    enum hart_id id = tp_r();
    systimer_tick[id]++;
}

void systimer_init( enum systimer_hz hz)
{
    s_timer_interrupt_disable();
    systimer_period(hz);
    enum hart_id id = tp_r();
    systimer_tick[id] = 0;
}

void systimer_start()
{
    systimer_reload();
    s_timer_interrupt_enable();
}

void systimer_pause()
{
    s_timer_interrupt_disable();
}
