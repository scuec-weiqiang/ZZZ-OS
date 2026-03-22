/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/irq.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 21:09:56
 * @LastEditTime : 2026-03-22 23:21:54
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
/**
 * @FilePath     : /ZZZ-OS/arch/arm/include/asm/irq.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 18:36:39
 * @LastEditTime : 2026-03-22 18:36:39
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/irqreturn.h>
#include <os/types.h>
#include <os/irq.h>
#include "armv7_gic.h"


void arch_irq_cpu_init() {

}

int arch_local_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id) {


}

int arch_extern_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id){
    

}
