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
#include <os/of_address.h>
// #include "armv7_gic.h"
#include <os/compiler_attributes.h>
#include <asm/irq.h>

#define FORCEDINLINE __attribute__((always_inline))
#define __ASM __asm     /* GNU C语言内嵌汇编关键字 */
#define __INLINE inline /* GNU内联关键字 */
#define __STATIC_INLINE static inline

#define __IM volatile const /* 只读 */
#define __OM volatile       /* 只写 */
#define __IOM volatile      /* 读写 */
#define __STRINGIFY(x) #x

#define __GIC_PRIO_BITS 5 /**< Number of Bits used for Priority Levels */

typedef struct {
    __IOM uint32_t D_CTLR; /*!< Offset: 0x1000 (R/W) Distributor Control Register */
    __IM uint32_t D_TYPER; /*!< Offset: 0x1004 (R/ )  Interrupt Controller Type Register */
    __IM uint32_t D_IIDR;  /*!< Offset: 0x1008 (R/ )  Distributor Implementer Identification Register */
    uint32_t RESERVED1[29];
    __IOM uint32_t D_IGROUPR[16]; /*!< Offset: 0x1080 - 0x0BC (R/W) Interrupt Group Registers */
    uint32_t RESERVED2[16];
    __IOM uint32_t D_ISENABLER[16]; /*!< Offset: 0x1100 - 0x13C (R/W) Interrupt Set-Enable Registers */
    uint32_t RESERVED3[16];
    __IOM uint32_t D_ICENABLER[16]; /*!< Offset: 0x1180 - 0x1BC (R/W) Interrupt Clear-Enable Registers */
    uint32_t RESERVED4[16];
    __IOM uint32_t D_ISPENDR[16]; /*!< Offset: 0x1200 - 0x23C (R/W) Interrupt Set-Pending Registers */
    uint32_t RESERVED5[16];
    __IOM uint32_t D_ICPENDR[16]; /*!< Offset: 0x1280 - 0x2BC (R/W) Interrupt Clear-Pending Registers */
    uint32_t RESERVED6[16];
    __IOM uint32_t D_ISACTIVER[16]; /*!< Offset: 0x1300 - 0x33C (R/W) Interrupt Set-Active Registers */
    uint32_t RESERVED7[16];
    __IOM uint32_t D_ICACTIVER[16]; /*!< Offset: 0x1380 - 0x3BC (R/W) Interrupt Clear-Active Registers */
    uint32_t RESERVED8[16];
    __IOM uint8_t D_IPRIORITYR[512]; /*!< Offset: 0x1400 - 0x5FC (R/W) Interrupt Priority Registers */
    uint32_t RESERVED9[128];
    __IOM uint8_t D_ITARGETSR[512]; /*!< Offset: 0x1800 - 0x9FC (R/W) Interrupt Targets Registers */
    uint32_t RESERVED10[128];
    __IOM uint32_t D_ICFGR[32]; /*!< Offset: 0x1C00 - 0xC7C (R/W) Interrupt configuration registers */
    uint32_t RESERVED11[32];
    __IM uint32_t D_PPISR;     /*!< Offset: 0x1D00 (R/ ) Private Peripheral Interrupt Status Register */
    __IM uint32_t D_SPISR[15]; /*!< Offset: 0x1D04 - 0xD3C (R/ ) Shared Peripheral Interrupt Status Registers */
    uint32_t RESERVED12[112];
    __OM uint32_t D_SGIR; /*!< Offset: 0x1F00 ( /W) Software Generated Interrupt Register */
    uint32_t RESERVED13[3];
    __IOM uint8_t D_CPENDSGIR[16]; /*!< Offset: 0x1F10 - 0xF1C (R/W) SGI Clear-Pending Registers */
    __IOM uint8_t D_SPENDSGIR[16]; /*!< Offset: 0x1F20 - 0xF2C (R/W) SGI Set-Pending Registers */
    uint32_t RESERVED14[40];
    __IM uint32_t D_PIDR4; /*!< Offset: 0x1FD0 (R/ ) Peripheral ID4 Register */
    __IM uint32_t D_PIDR5; /*!< Offset: 0x1FD4 (R/ ) Peripheral ID5 Register */
    __IM uint32_t D_PIDR6; /*!< Offset: 0x1FD8 (R/ ) Peripheral ID6 Register */
    __IM uint32_t D_PIDR7; /*!< Offset: 0x1FDC (R/ ) Peripheral ID7 Register */
    __IM uint32_t D_PIDR0; /*!< Offset: 0x1FE0 (R/ ) Peripheral ID0 Register */
    __IM uint32_t D_PIDR1; /*!< Offset: 0x1FE4 (R/ ) Peripheral ID1 Register */
    __IM uint32_t D_PIDR2; /*!< Offset: 0x1FE8 (R/ ) Peripheral ID2 Register */
    __IM uint32_t D_PIDR3; /*!< Offset: 0x1FEC (R/ ) Peripheral ID3 Register */
    __IM uint32_t D_CIDR0; /*!< Offset: 0x1FF0 (R/ ) Component ID0 Register */
    __IM uint32_t D_CIDR1; /*!< Offset: 0x1FF4 (R/ ) Component ID1 Register */
    __IM uint32_t D_CIDR2; /*!< Offset: 0x1FF8 (R/ ) Component ID2 Register */
    __IM uint32_t D_CIDR3; /*!< Offset: 0x1FFC (R/ ) Component ID3 Register */

    __IOM uint32_t C_CTLR;  /*!< Offset: 0x2000 (R/W) CPU Interface Control Register */
    __IOM uint32_t C_PMR;   /*!< Offset: 0x2004 (R/W) Interrupt Priority Mask Register */
    __IOM uint32_t C_BPR;   /*!< Offset: 0x2008 (R/W) Binary Point Register */
    __IM uint32_t C_IAR;    /*!< Offset: 0x200C (R/ ) Interrupt Acknowledge Register */
    __OM uint32_t C_EOIR;   /*!< Offset: 0x2010 ( /W) End Of Interrupt Register */
    __IM uint32_t C_RPR;    /*!< Offset: 0x2014 (R/ ) Running Priority Register */
    __IM uint32_t C_HPPIR;  /*!< Offset: 0x2018 (R/ ) Highest Priority Pending Interrupt Register */
    __IOM uint32_t C_ABPR;  /*!< Offset: 0x201C (R/W) Aliased Binary Point Register */
    __IM uint32_t C_AIAR;   /*!< Offset: 0x2020 (R/ ) Aliased Interrupt Acknowledge Register */
    __OM uint32_t C_AEOIR;  /*!< Offset: 0x2024 ( /W) Aliased End Of Interrupt Register */
    __IM uint32_t C_AHPPIR; /*!< Offset: 0x2028 (R/ ) Aliased Highest Priority Pending Interrupt Register */
    uint32_t RESERVED15[41];
    __IOM uint32_t C_APR0; /*!< Offset: 0x20D0 (R/W) Active Priority Register */
    uint32_t RESERVED16[3];
    __IOM uint32_t C_NSAPR0; /*!< Offset: 0x20E0 (R/W) Non-secure Active Priority Register */
    uint32_t RESERVED17[6];
    __IM uint32_t C_IIDR; /*!< Offset: 0x20FC (R/ ) CPU Interface Identification Register */
    uint32_t RESERVED18[960];
    __OM uint32_t C_DIR; /*!< Offset: 0x3000 ( /W) Deactivate Interrupt Register */
} GIC_Type;

FORCEDINLINE __STATIC_INLINE void GIC_Init(void *gicBase) {
    uint32_t i;
    uint32_t irqRegs;
    GIC_Type *gic = (GIC_Type *)gicBase;

    irqRegs = (gic->D_TYPER & 0x1FUL) + 1;

    /* On POR, all SPI is in group 0, level-sensitive and using 1-N model */

    /* Disable all PPI, SGI and SPI */
    for (i = 0; i < irqRegs; i++)
        gic->D_ICENABLER[i] = 0xFFFFFFFFUL;

    /* Make all interrupts have higher priority */
    gic->C_PMR = (0xFFUL << (8 - __GIC_PRIO_BITS)) & 0xFFUL;

    /* No subpriority, all priority level allows preemption */
    gic->C_BPR = 7 - __GIC_PRIO_BITS;

    /* Enable group0 distribution */
    gic->D_CTLR = 1UL;

    /* Enable group0 signaling */
    gic->C_CTLR = 1UL;
}

struct gic_data {
    GIC_Type *base;
    struct device_node *np;
    struct irq_chip *chip;
    struct irq_domain *domain;
} gic_data;

static int armv7_gic_claim(struct irq_chip *self)
{
    return (gic_data.base->C_IAR & 0x1FFFUL);
}

static void armv7_gic_eoi(struct irq_chip *self, int hwirq)
{
    gic_data.base->C_EOIR = hwirq;
}

static void armv7_gic_enable(struct irq_chip *self, int hwirq)
{
    gic_data.base->D_ISENABLER[((uint32_t)(int32_t)hwirq) >> 5] = (uint32_t)(1UL << (((uint32_t)(int32_t)hwirq) & 0x1FUL));
}

static void armv7_gic_disable(struct irq_chip *self, int hwirq)
{
    gic_data.base->D_ICENABLER[((uint32_t)(int32_t)hwirq) >> 5] = (uint32_t)(1UL << (((uint32_t)(int32_t)hwirq) & 0x1FUL));
}

static void armv7_gic_set_pending(struct irq_chip *self, int hwirq)
{
    
    gic_data.base->D_ISPENDR[hwirq >> 5] = 1U << (hwirq & 31);
}

static void armv7_gic_clear_pending(struct irq_chip *self, int hwirq)
{
    gic_data.base->D_ICPENDR[hwirq >> 5] = 1U << (hwirq & 31);
}

static int armv7_gic_get_pending(struct irq_chip *self, int hwirq)
{

    return !!(gic_data.base->D_ISPENDR[hwirq >> 5] & (1U << (hwirq & 31)));
}

static void armv7_gic_set_priority(struct irq_chip *self, int hwirq, int priority)
{
    gic_data.base->D_IPRIORITYR[((uint32_t)(int32_t)hwirq)] = (uint8_t)((priority << (8UL - __GIC_PRIO_BITS)) & (uint32_t)0xFFUL);
}

static int armv7_gic_get_priority(struct irq_chip *self, int hwirq)
{
    return  (((uint32_t)gic_data.base->D_IPRIORITYR[((uint32_t)(int32_t)hwirq)] >> (8UL - __GIC_PRIO_BITS)));
}

static void armv7_gic_cpu_enable(struct irq_chip *self, int cpu)
{
    gic_data.base->C_PMR = 0xFF;
    gic_data.base->C_BPR = 7 - __GIC_PRIO_BITS;
    gic_data.base->C_CTLR = 1;
}

static void armv7_gic_set_prio_mask(struct irq_chip *self, int cpu, int prio)
{
    gic_data.base->C_PMR = prio & 0xFF;
}

static void armv7_gic_send_ipi(struct irq_chip *self, int target_cpu, int ipi_id)
{
    
    gic_data.base->D_SGIR = ((1U << target_cpu) << 16) | (ipi_id & 0xF);
}

static void armv7_gic_broadcast_ipi(struct irq_chip *self, int ipi_id)
{
    gic_data.base->D_SGIR = (1U << 24) | (ipi_id & 0xF);
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

reg_t handle_gic_irq(reg_t *ctx) {
    int hwirq = gic_data.chip->ops->cpuif->claim(gic_data.chip);
    // printk("GIC IRQ: hwirq=%d\n", hwirq);
    int virq = irq_domain_get_virq(gic_data.np, hwirq);
    // printk("GIC IRQ: virq=%d\n", virq);
    if (virq >= 0) {
        do_irq(*ctx, (void *)virq);
        gic_data.chip->ops->cpuif->eoi(gic_data.chip, hwirq);
    } else {
        printk("Invalid virq!\n");
    }
    return 0;
}

static int gic_of_init(struct device_node *np, struct device_node *parent)
{
    gic_data.base = of_iomap(np, 0);
    printk("gic base: %xu\n", gic_data.base);
    unsigned int irq_count;
    int virq_base;
    struct irq_chip *chip;
    struct irq_domain *domain;

    chip = irq_chip_register(np, &armv7_gic_chip_ops,  NULL);

    if (!chip)
        return -1;

    irq_count = (((gic_data.base->D_TYPER & 0x1F) + 1) * 32);
    
    virq_base = irq_domain_alloc_virq_base(irq_count);
    domain = irq_domain_create(np, virq_base, irq_count);
    if (!domain)
        return -1;

    gic_data.np = np;
    gic_data.chip = chip;
    gic_data.domain = domain;

    GIC_Init(gic_data.base);
    set_handle_irq(handle_gic_irq);
    return 0;
}

IRQCHIP_DECLARE(arm_gic, "arm,cortex-a7-gic", gic_of_init);
