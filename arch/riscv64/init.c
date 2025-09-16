/**
 * @FilePath: /ZZZ/arch/riscv64/init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 00:43:47
 * @LastEditTime: 2025-09-16 21:45:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "riscv.h"
#include "systimer.h"
#include "interrupt.h"
#include "trap_handler.h"
#include "clint.h" 
#include "printf.h"
extern void init_kernel();
void init()
{
    satp_w(0);
    medeleg_w(medeleg_r()|0xffffffff);// 将所有异常委托给S模式处理
    mideleg_w(mideleg_r()|(1<<1)|(1<<9)); // 将s模式软件中断，外部中断委托给s模式
    
    printf("mideleg is %x\n",mideleg_r());
    // 将整个用户空间（39位）设置保护
    pmpaddr0_w(0xffffffffffffffffull);
    pmpcfg0_w(0xf);
    
    trap_init();
    // systimer_init(mhartid_r(),SYS_HZ_1000);

    // 使能S模式外部中断，定时器中断和软件中断
    s_extern_interrupt_enable();
    s_soft_interrupt_enable();


    systimer_init(mhartid_r(),SYS_HZ_1000);

    M_TO_S(init_kernel);
}