/*******************************************************************************************
 * @FilePath     : /ZZZ/arch/riscv64/qemu_virt/systimer.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-20 00:15:52
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#ifndef __HWTIMER_H
#define __HWTIMER_H

#include "types.h"
#include "platform.h"

#define tick_s      SYS_CLOCK_FREQ
#define tick_ms     (tick_s/1000)
#define tick_us     (tick_ms/1000)

enum SYS_CONFIG_HZ{
    SYS_HZ_1000 = 1*tick_ms,
    SYS_HZ_250 = 4*tick_ms,
    SYS_HZ_100 = 10*tick_ms,
};

extern enum SYS_CONFIG_HZ systimer_hz;
extern uint64_t systimer_tick;
extern void systimer_init(enum SYS_CONFIG_HZ hz);
extern void systimer_load(uint64_t hartid,uint64_t value);
extern uint64_t systimer_get_time();

#endif 