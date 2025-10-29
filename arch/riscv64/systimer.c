/**
 * @FilePath: /ZZZ-OS/arch/riscv64/systimer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-10-29 21:45:20
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "os/types.h"
#include "asm/platform.h"
#include "asm/interrupt.h"
#include "asm/clint.h" 
#include "asm/systimer.h"
#include "asm/riscv.h"

//系统时钟以0核为基准
static uint64_t systimer_tick[MAX_HARTS_NUM] = {0};
static enum systimer_hz systimer_hz[MAX_HARTS_NUM] = {SYS_HZ_1,SYS_HZ_1};

inline void systimer_period(enum systimer_hz hz)
{
    enum hart_id id = tp_r();
    systimer_hz[id] = hz;
}

inline void systimer_reload()
{   
    enum hart_id id = tp_r();
    uint64_t temp = time_r();
    temp +=  systimer_hz[id];
    stimecmp_w(temp);
}

inline uint64_t systick()
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
