/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/systimer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-09-21 00:01:50
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
u64 systimer_tick = 0;
enum systimer_hz systimer_hz[MAX_HARTS_NUM] = {SYS_HZ_100,SYS_HZ_100};


void systimer_init(enum hart_id hart_id, enum systimer_hz hz)
{
    systimer_hz[hart_id] = hz;
    systimer_load(hart_id,(u64)hz);
    s_timer_interrupt_enable();
}

void systimer_load(enum hart_id hartid,u64 value)
{   
    u64 temp = __clint_mtime_get();
    temp += value;
    __clint_mtimecmp_set(hartid,temp);
}

u64 systimer_get_time()
{
    return __clint_mtime_get();
}