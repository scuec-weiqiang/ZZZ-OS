/*******************************************************************************************
 * @FilePath     : /ZZZ/arch/riscv64/qemu_virt/hwtimer.h
 * @Description  : 内核硬件定时器头文件 
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-18 19:38:44
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#ifndef __HWTIMER_H
#define __HWTIMER_H

#include "types.h"
#include "platform.h"

extern void _hwtimer_load(uint64_t value);

#define tick_s  SYS_CLOCK_FREQ
#define tick_ms (tick_s/1000)
#define tick_us (tick_ms/1000)

#define hwtimer_reload_s(x)  _hwtimer_load(tick_s*(x))
#define hwtimer_reload_ms(x) _hwtimer_load(tick_ms*(x))
#define hwtimer_reload_us(x) _hwtimer_load(tick_us*(x))


extern uint64_t hwtimer_tick;

#endif 