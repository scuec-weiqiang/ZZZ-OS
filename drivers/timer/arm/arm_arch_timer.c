/**
 * @FilePath: /ZZZ-OS/drivers/timer/arm/arm_arch_timer.c
 * @Description: ARMv7 Generic Timer driver
 */
#include <asm/arch_timer.h>
#include <asm/irq.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/of_irq.h>
#include <os/printk.h>
#include <os/time.h>
#include <os/timer_chip.h>
#include <os/utils.h>

struct arm_arch_timer_data {
    uint32_t cntfrq;
    uint32_t period_cycles;
    int virq;
    uint64_t tick;
};

static struct arm_arch_timer_data arm_timer = {
    .cntfrq = 0,
    .period_cycles = 0,
    .virq = -1,
    .tick = 0,
};

static inline uint32_t arm_cntfrq_read(void) {
    uint32_t v = 0;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(v));
    return v;
}

static inline void arm_cntp_tval_write(uint32_t v) {
    asm volatile("mcr p15, 0, %0, c14, c2, 0" : : "r"(v));
}

static inline uint64_t arm_cntpct_read(void) {
    uint32_t lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static inline uint32_t arm_cntp_ctl_read(void) {
    uint32_t v = 0;
    asm volatile("mrc p15, 0, %0, c14, c2, 1" : "=r"(v));
    return v;
}

static inline void arm_cntp_ctl_write(uint32_t v) {
    asm volatile("mcr p15, 0, %0, c14, c2, 1" : : "r"(v));
}

static irqreturn_t arm_arch_timer_irq_handler(int virq, void *dev_id) {
    (void)virq;
    (void)dev_id;
    arch_timer_reload();
    printk("tick=%d%du\r",
       (uint32_t)(arm_timer.tick >> 32),
       (uint32_t)arm_timer.tick);

    systick_up();
    return IRQ_HANDLED;
}

void arch_timer_period(enum arch_timer_hz hz) {
    uint32_t rate = arm_timer.cntfrq;
    uint32_t h = (uint32_t)hz;

    if (rate == 0) {
        rate = arm_cntfrq_read();
        arm_timer.cntfrq = rate;
    }
    if (h == 0) {
        h = 100;
    }

    arm_timer.period_cycles = div_u32(rate, h);
    if (arm_timer.period_cycles == 0) {
        arm_timer.period_cycles = 1;
    }
}

void arch_timer_reload(void) {
    arm_cntp_tval_write(arm_timer.period_cycles);
}

void arch_timer_init(enum arch_timer_hz hz) {
    arm_timer.cntfrq = arm_cntfrq_read();
    arm_timer.tick = 0;
    arch_timer_period(hz);
}

void arch_timer_start(void) {
    uint32_t ctl;

    arch_timer_reload();
    ctl = arm_cntp_ctl_read();
    ctl &= ~(1U << 1); /* IMASK = 0 */
    ctl |= 1U;         /* ENABLE = 1 */
    arm_cntp_ctl_write(ctl);

    if (arm_timer.virq >= 0) {
        irq_enable(arm_timer.virq);
    }
}

void arch_timer_pause(void) {
    uint32_t ctl = arm_cntp_ctl_read();
    ctl &= ~1U; /* ENABLE = 0 */
    arm_cntp_ctl_write(ctl);

    if (arm_timer.virq >= 0) {
        irq_disable(arm_timer.virq);
    }
}

uint64_t systick(void) {
    return arm_timer.tick;
}

void systick_up(void) {
    arm_timer.tick++;
}

uint64_t arch_timer_counter(void) {
    return arm_cntpct_read();
}

uint32_t arch_timer_frequency(void) {
    if (arm_timer.cntfrq == 0) {
        arm_timer.cntfrq = arm_cntfrq_read();
    }
    return arm_timer.cntfrq;
}

static int arm_arch_timer_of_init(struct device_node *np, struct device_node *parent) {
    int virq;
    int cpu;

    (void)parent;

    virq = of_irq_get(np, 0);
    if (virq < 0) {
        printk("arm_arch_timer: get virq failed\n");
        return -1;
    }

    cpu = arch_irq_cpu_id();
    if (irq_percpu_request(virq, cpu, arm_arch_timer_irq_handler, "arm_arch_timer", NULL) < 0) {
        printk("arm_arch_timer: percpu request failed\n");
        return -1;
    }
    
    arm_timer.virq = virq;
    arch_timer_init(SYS_HZ_1);
    if (timekeeping_register_clocksource(arch_timer_counter, arch_timer_frequency) < 0) {
        printk("arm_arch_timer: register clocksource failed\n");
        return -1;
    }
    arch_timer_start();
    printk("arm_arch_timer: initialized with virq=%d\n", virq);
    return 0;
}

TIMERCHIP_DECLARE(arm_arch_timer, "arm,armv7-timer", arm_arch_timer_of_init);
