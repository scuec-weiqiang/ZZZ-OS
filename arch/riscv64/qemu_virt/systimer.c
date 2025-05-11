/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/systimer.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 21:58:52
 * @LastEditTime: 2025-05-10 18:06:48
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
uint64_t systimer_tick = 0;
SYS_CONFIG_HZ_t systimer_hz[MAX_HARTS_NUM] = {SYS_HZ_100,SYS_HZ_100};

extern void machine_timer_trap_entry(void); //定义在trap.S文件中
extern uint8_t _systimer_ctx[5*8*MAX_HARTS_NUM]; //定义在链接文件中，用来保存定时器的一些信息
reg_t (*systimer_ctx)[5] = (reg_t (*)[5])_systimer_ctx;

void systimer_init(hart_id_t hart_id, enum SYS_CONFIG_HZ hz)
{
    systimer_hz[hart_id] = hz;
    systimer_load(hart_id,(uint64_t)hz);

    reg_t *clint_mtimecmp = (reg_t*)CLINT_MTIMECMP_BASE;
    systimer_ctx[hart_id][3] = (reg_t)&clint_mtimecmp[hart_id];
    systimer_ctx[hart_id][4] = (reg_t)systimer_hz[hart_id];

    mtvec_w((reg_t)machine_timer_trap_entry);
    mscratch_w(&systimer_ctx[hart_id]);
    m_timer_interrupt_enable();
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