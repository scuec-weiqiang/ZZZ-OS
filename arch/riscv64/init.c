/**
 * @FilePath: /ZZZ/arch/riscv64/init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 00:43:47
 * @LastEditTime: 2025-05-08 00:40:18
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "riscv.h"
#include "systimer.h"
#include "interrupt.h"
extern void init_kernel();

void init()
{
    satp_w(0);

    // 将所有异常和中断委托给S模式处理
    medeleg_w(0xffff);
    mideleg_w(0xffff);
    
    // 使能S模式外部中断，定时器中断和软件中断
    s_extern_interrupt_enable();
    // s_timer_interrupt_enable();
    s_soft_interrupt_enable();

    // 将整个用户空间（39位）设置保护
    pmpaddr0_w(0x3ffffffffffffful);
    pmpcfg0_w(0xf);
    
    systimer_init(mhartid_r(),SYS_HZ_100);
    m_global_interrupt_enable();    
    
    M_TO_S(init_kernel);
    
}