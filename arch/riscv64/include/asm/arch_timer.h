/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/arch_timer.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-17 00:52:26
 * @LastEditTime: 2025-10-30 21:31:39
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef __HWTIMER_H
#define __HWTIMER_H

#include <os/types.h>
#include <asm/platform.h>

#define tick_s      SYS_CLOCK_FREQ
#define tick_ms     (tick_s/1000)
#define tick_us     (tick_ms/1000)

enum arch_timer_hz {
    SYS_HZ_1000 = 1*tick_ms,
    SYS_HZ_250 = 4*tick_ms,
    SYS_HZ_100 = 10*tick_ms,
    SYS_HZ_50 = 20*tick_ms,
    SYS_HZ_20 = 50*tick_ms,
    SYS_HZ_10 = 100*tick_ms,
    SYS_HZ_5 = 200*tick_ms,
    SYS_HZ_2 = 500*tick_ms,
    SYS_HZ_1 = 1000*tick_ms,
};

extern void arch_timer_init(enum arch_timer_hz hz);
extern void arch_timer_start();
extern void arch_timer_pause();
extern void arch_timer_period(enum arch_timer_hz hz);
extern void arch_timer_reload();
extern uint64_t systick();
extern void systick_up();

#endif 