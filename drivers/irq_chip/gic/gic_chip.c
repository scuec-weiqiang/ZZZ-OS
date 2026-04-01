/**
 * @FilePath     : /ZZZ-OS/drivers/irq_chip/gic/gic_chip.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-18 16:15:46
 * @LastEditTime : 2026-03-26 20:00:32
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/of.h>
#include <os/init.h>
#include <os/device.h>
#include "armv7_gic.h"


static inline GIC_Type *armv7_gic_base(void)
{
    return (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
}

static int armv7_gic_claim(struct irq_chip *self)
{
    return GIC_AcknowledgeIRQ();
}

static void armv7_gic_eoi(struct irq_chip *self, int hwirq)
{
    GIC_DeactivateIRQ((uint32_t)hwirq);
}

static void armv7_gic_enable(struct irq_chip *self, int hwirq)
{
    GIC_EnableIRQ((IRQn_Type)hwirq);
}

static void armv7_gic_disable(struct irq_chip *self, int hwirq)
{
    GIC_DisableIRQ((IRQn_Type)hwirq);
}

static void armv7_gic_set_pending(struct irq_chip *self, int hwirq)
{
    GIC_Type *gic = armv7_gic_base();
    gic->D_ISPENDR[hwirq >> 5] = 1U << (hwirq & 31);
}

static void armv7_gic_clear_pending(struct irq_chip *self, int hwirq)
{
    GIC_Type *gic = armv7_gic_base();
    gic->D_ICPENDR[hwirq >> 5] = 1U << (hwirq & 31);
}

static int armv7_gic_get_pending(struct irq_chip *self, int hwirq)
{
    GIC_Type *gic = armv7_gic_base();
    return !!(gic->D_ISPENDR[hwirq >> 5] & (1U << (hwirq & 31)));
}

static void armv7_gic_set_priority(struct irq_chip *self, int hwirq, int priority)
{
    GIC_SetPriority((IRQn_Type)hwirq, (uint32_t)priority);
}

static int armv7_gic_get_priority(struct irq_chip *self, int hwirq)
{
    return (int)GIC_GetPriority((IRQn_Type)hwirq);
}

static void armv7_gic_cpu_enable(struct irq_chip *self, int cpu)
{
    GIC_Type *gic = armv7_gic_base();
    gic->C_PMR = 0xFF;
    gic->C_BPR = 7 - __GIC_PRIO_BITS;
    gic->C_CTLR = 1;
    gic->D_CTLR = 1;
}

static void armv7_gic_set_prio_mask(struct irq_chip *self, int cpu, int prio)
{
    GIC_Type *gic = armv7_gic_base();
    gic->C_PMR = prio & 0xFF;
}

static void armv7_gic_send_ipi(struct irq_chip *self, int target_cpu, int ipi_id)
{
    GIC_Type *gic = armv7_gic_base();
    gic->D_SGIR = ((1U << target_cpu) << 16) | (ipi_id & 0xF);
}

static void armv7_gic_broadcast_ipi(struct irq_chip *self, int ipi_id)
{
    GIC_Type *gic = armv7_gic_base();
    gic->D_SGIR = (1U << 24) | (ipi_id & 0xF);
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


struct irq_cpuif_ops armv7_gic_cpuif_ops = {
    .claim = armv7_gic_claim,
    .eoi = armv7_gic_eoi,
    .cpu_enable = armv7_gic_cpu_enable,
    .set_prio_mask = armv7_gic_set_prio_mask,
};



struct irq_ipi_ops armv7_gic_ipi_ops = {
    .send_ipi = armv7_gic_send_ipi,
    .broadcast_ipi = armv7_gic_broadcast_ipi,
};


struct irq_chip_ops armv7_gic_chip_ops = {
    .irq = &armv7_gic_irq_ops,
    .cpuif = &armv7_gic_cpuif_ops,
    .ipi = &armv7_gic_ipi_ops,
};

static int gic_of_init(struct device_node *np, struct device_node *parent)
{
    printk("Initializing ARM GIC...\n");
    // GIC_Type *gic = armv7_gic_base();
    // unsigned int irq_count;
    // int virq_base;
    // struct irq_chip *chip;

    // chip = irq_chip_register("arm-gic", &armv7_gic_chip_ops, 0, NULL);

    // if (!chip)
    //     return -1;

    // irq_count = (((gic->D_TYPER & 0x1F) + 1) * 32);
    // virq_base = irq_domain_alloc_virq_base(irq_count);

    // if (!irq_domain_create(chip, virq_base, irq_count))
    //     return -1;

    // GIC_Init();
    return 0;
}

// reg_t handle_gic_irq(reg_t *_ctx) {
//     struct trap_frame *ctx = (struct trap_frame *)_ctx;
//     reg_t return_epc = ctx->sepc;
//       struct irq_chip *chip = irq_chip_lookup("arm-gic", 0);
//         if (chip) {
//             int hwirq = chip->ops->cpuif->claim(chip);
//             int virq = irq_domain_get_virq(chip, hwirq);
//             if (virq >= 0) {
//                 do_irq(*ctx, (void *)(uintptr_t)virq);
//             } else {
//                 printk("Invalid virq!\n");
//             }
//         } else {
//             printk("IRQ chip not found!\n");
//         }
// }


IRQCHIP_DECLARE(arm_gic, "arm,cortex-a7-gic", gic_of_init);
