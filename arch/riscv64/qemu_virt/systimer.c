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
static enum systimer_hz systimer_hz[MAX_HARTS_NUM] = {SYS_HZ_100,SYS_HZ_100};


void systimer_period(enum hart_id id, enum systimer_hz hz)
{
    systimer_hz[id] = hz;
}

void systimer_load(enum hart_id id)
{   
    u64 temp = time_r();
    temp +=  systimer_hz[id];
    stimecmp_w(temp);
}

u64 systick(enum hart_id id)
{
    return systimer_tick[id];
}

void systick_up(enum hart_id id)
{
    systimer_tick[id]++;
}

void systimer_init(enum hart_id id, enum systimer_hz hz)
{
    systimer_period(id,hz);
    systimer_load(id);
    s_timer_interrupt_enable();
}
