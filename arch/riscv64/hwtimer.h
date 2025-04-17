/*******************************************************************************************
 * @FilePath     : /ZZZ/arch/riscv64/hwtimer.h
 * @Description  : 内核硬件定时器头文件 
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-17 01:48:54
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#ifndef __HWTIMER_H
#define __HWTIMER_H

#include "types.h"
#include "platform.h"

extern void hwtimer_load(uint64_t value);

#define tick_ms 10000
#define tick_us 10

#define hwtimer_ms(x) hwtimer_load(tick_ms*x)
#define hwtimer_us(x) hwtimer_load(tick_us*x)

extern uint64_t hwtimer_tick;

#endif 