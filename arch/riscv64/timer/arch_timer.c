/**
 * @FilePath: /ZZZ-OS/arch/riscv64/timer/arch_timer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-11-14 15:21:31
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include <os/types.h>
#include <asm/platform.h>
#include <asm/interrupt.h>
#include <asm/arch_timer.h>
#include <asm/riscv.h>
#include <os/irq.h>

//系统时钟以0核为基准
static uint64_t arch_timer_tick[] = {0};
static enum arch_timer_hz arch_timer_hz[] = {SYS_HZ_1,SYS_HZ_1};

void arch_timer_period(enum arch_timer_hz hz)
{
    int id = tp_r();
    arch_timer_hz[id] = hz;
}

void arch_timer_reload()
{   
    int id = tp_r();
    uint64_t temp = time_r();
    temp +=  arch_timer_hz[id];
    stimecmp_w(temp);
}

uint64_t systick()
{
    int id = tp_r();
    return arch_timer_tick[id];
}

void systick_up()
{
    int id = tp_r();
    arch_timer_tick[id]++;
}

void arch_timer_init( enum arch_timer_hz hz)
{
    s_timer_interrupt_disable();
    arch_timer_period(hz);
    int id = tp_r();
    arch_timer_tick[id] = 0;
}

void arch_timer_start()
{
    arch_timer_reload();
    irq_enable(TIMER_IRQ);
}

void arch_timer_pause()
{
    irq_disable(TIMER_IRQ);
}
