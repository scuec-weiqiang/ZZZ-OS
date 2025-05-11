/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/systimer.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-17 00:52:26
 * @LastEditTime: 2025-05-09 22:05:05
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

typedef enum SYS_CONFIG_HZ{
    SYS_HZ_1000 = 1*tick_ms,
    SYS_HZ_250 = 4*tick_ms,
    SYS_HZ_100 = 10*tick_ms,
}SYS_CONFIG_HZ_t;

extern SYS_CONFIG_HZ_t systimer_hz[MAX_HARTS_NUM];
extern uint64_t systimer_tick;
extern void systimer_init(hart_id_t hart_id,enum SYS_CONFIG_HZ hz);
extern void systimer_load(hart_id_t hartid,uint64_t value);
extern uint64_t systimer_get_time();

#endif 