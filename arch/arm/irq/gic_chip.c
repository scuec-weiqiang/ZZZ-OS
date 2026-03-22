/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/gic_chip.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-18 16:15:46
 * @LastEditTime : 2026-03-22 23:23:21
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/of.h>
#include <os/module.h>
#include <os/driver_model.h>
#include "armv7_gic.h"

// #include <asm/trap_handler.h>
// #include <asm/interrupt.h>

static int armv7_gic_claim(struct irq_chip *self) {
   return  GIC_AcknowledgeIRQ();
}

static void armv7_gic_eoi(struct irq_chip *self, int hwirq) {   
//    GIC_EndOfInterrupt(hwirq);
}

static void armv7_gic_enable(struct irq_chip* self, int hwirq) {
   
}

static void armv7_gic_disable(struct irq_chip* self, int hwirq) {
   
}

static void armv7_gic_set_pending(struct irq_chip* self, int hwirq) {
   
}

static void armv7_gic_clear_pending(struct irq_chip* self, int hwirq) {
    
}

static int armv7_gic_get_pending(struct irq_chip *self, int hwirq) {
    int ret = 0;

    return ret;
}


static void armv7_gic_set_priority(struct irq_chip* self, int hwirq, int priority) {
}


static int armv7_gic_get_priority(struct irq_chip* self, int hwirq) {
    return 0;
}

struct irq_ops armv7_gic_irq_ops = {
    .enable = armv7_gic_enable,
    .disable = armv7_gic_disable,
    .set_pending = armv7_gic_set_pending,
    .clear_pending = armv7_gic_clear_pending,
    .get_pending = armv7_gic_get_pending,
    .set_priority = armv7_gic_set_priority, 
    .get_priority = armv7_gic_get_priority, 
};


static void armv7_gic_cpu_enable(struct irq_chip *self, int cpu) {
    
}


static void armv7_gic_set_prio_mask(struct irq_chip *self, int cpu, int prio) {
   
}


struct irq_cpuif_ops armv7_gic_cpuif_ops = {
    .claim = armv7_gic_claim,
    .eoi = armv7_gic_eoi,
    .cpu_enable = armv7_gic_cpu_enable,
    .set_prio_mask = armv7_gic_set_prio_mask,
};


static void armv7_gic_send_ipi(struct irq_chip *self, int target_cpu, int ipi_id) {
    
}

static void armv7_gic_broadcast_ipi(struct irq_chip *self, int ipi_id) {
    
}

struct irq_ipi_ops armv7_gic_ipi_ops = {
    .send_ipi = armv7_gic_send_ipi,
    .broadcast_ipi = armv7_gic_broadcast_ipi,
};


struct irq_chip_ops armv7_gic_chip_ops = {
    .irq = &armv7_gic_irq_ops,
    .cpuif = &armv7_gic_cpuif_ops,
    .ipi = &armv7_gic_ipi_ops,
};

static int gic_probe(struct device *pdev)

