/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/systimer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-05-01 22:34:56
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "types.h"
#include "platform.h"
#include "interrupt.h"
#include "clint.h" 
#include "systimer.h"

uint64_t systimer_tick = 0;
enum SYS_CONFIG_HZ systimer_hz = SYS_HZ_100;

void systimer_init(hart_id_t hart_id, enum SYS_CONFIG_HZ hz)
{
    systimer_hz = hz;
    systimer_load(hart_id,(uint64_t)hz); //系统时钟以0核为基准
    timer_interrupt_enable();
}

void systimer_load(hart_id_t hartid,uint64_t value)
{   
    uint64_t temp = __clint_mtime_get();
    temp += value;
    __clint_mtimecmp_set(hartid,temp);
}

uint64_t systimer_get_time()
{
    return __clint_mtime_get();
}