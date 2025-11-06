/**
 * @FilePath: /ZZZ-OS/kernel/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 14:53:42
 * @LastEditTime: 2025-11-01 13:40:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq.h>
#include <asm/irq.h>

#define IRQ_COUNT 128
struct irq_desc irq_desc[IRQ_COUNT];

void irq_init(void) {
    for (int i=0; i<IRQ_COUNT; i++) {
        irq_desc[i].name = NULL;
        irq_desc[i].virq = i;
        irq_desc[i].hwirq = -1;
    }
}
 
void irq_register(struct irq_desc *desc) {

}

void irq_enable(int virq) {

}

void irq_disable(int virq) {

}

void irq_acknowledge(int virq) {

}

void irq_set_priority(int, int priority) {

}

int irq_get_priority(int) {

}

reg_t do_irq(reg_t ctx) {

}