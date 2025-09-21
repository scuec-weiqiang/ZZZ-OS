/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/systimer.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-17 00:52:26
 * @LastEditTime: 2025-09-21 00:04:28
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef __HWTIMER_H
#define __HWTIMER_H

#include "types.h"
#include "platform.h"

#define tick_s      SYS_CLOCK_FREQ
#define tick_ms     (tick_s/1000)
#define tick_us     (tick_ms/1000)

enum systimer_hz{
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

extern enum systimer_hz systimer_hz[MAX_HARTS_NUM];
extern u64 systimer_tick;
extern void systimer_init(enum hart_id hart_id,enum systimer_hz hz);
extern void systimer_load(enum hart_id hartid,u64 value);
extern u64 systimer_get_time();

#endif 