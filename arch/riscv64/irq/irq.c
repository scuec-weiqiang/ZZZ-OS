/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq_ctrl.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:15:44
 * @LastEditTime: 2025-10-31 15:10:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <arch/irq.h>
#include <asm/trap_handler.h>
#include <asm/interrupt.h>

static void riscv64_irq_ctrl_init(void) {
    // 初始化中断控制器的代码
    trap_init();
}

static void riscv64_irq_ctrl_enable(int irq) {
    // 使能指定中断号的代码
}