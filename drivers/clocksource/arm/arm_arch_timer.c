/**
 * @FilePath: /ZZZ-OS/drivers/timer/arm/arm_arch_timer.c
 * @Description: ARMv7 Generic Timer driver
 */
// #include <asm/irq.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/of_irq.h>
#include <os/printk.h>
#include <os/timekeeping.h>
#include <os/timer_chip.h>
#include <os/cpu.h>
#include <os/utils.h>

struct arm_arch_timer_data {
    u32 cntfrq;
    bool active;
    int virq;
};

static struct arm_arch_timer_data arm_timer = {
    .cntfrq = 0,
    .virq = -1,
};

static inline u32 arm_cntfrq_read(void) {
    u32 v = 0;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(v));
    return v;
}

static inline void arm_cntp_tval_write(u32 v) {
    asm volatile("mcr p15, 0, %0, c14, c2, 0" : : "r"(v));
}

static inline u64 arm_cntpct_read(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return ((u64)hi << 32) | (u64)lo;
}

static inline u32 arm_cntp_ctl_read(void) {
    u32 v = 0;
    asm volatile("mrc p15, 0, %0, c14, c2, 1" : "=r"(v));
    return v;
}

static inline void arm_cntp_ctl_write(u32 v) {
    asm volatile("mcr p15, 0, %0, c14, c2, 1" : : "r"(v));
}

// static u32 arch_timer_frequency(void) {
//     if (arm_timer.cntfrq == 0) {
//         arm_timer.cntfrq = arm_cntfrq_read();
//     }
//     return arm_timer.cntfrq;
// }

static irqreturn_t arm_arch_timer_irq_handler(int virq, void *dev_id) {
    (void)virq;
    (void)dev_id;

    timekeeping_timer_interrupt();

    return IRQ_HANDLED;
}

// void arch_timer_period(u32 hz) {
//     u32 rate = arm_timer.cntfrq;

//     if (rate == 0) {
//         rate = arm_cntfrq_read();
//         arm_timer.cntfrq = rate;
//     }
//     if (hz == 0) {
//         hz = 100;
//     }

//     arm_timer.period_cycles = div_u32(rate, hz);
//     if (arm_timer.period_cycles == 0) {
//         arm_timer.period_cycles = 1;
//     }
// }


static void arch_timer_set_next_event(u64 delta_ns) {
    if (arm_timer.active==false) {
        u32 ctl = arm_cntp_ctl_read();
        ctl &= ~(1U << 1); /* IMASK = 0 */
        ctl |= 1U;         /* ENABLE = 1 */
        arm_cntp_ctl_write(ctl);

        if (arm_timer.virq >= 0) {
            irq_enable(arm_timer.virq);
        } 
        arm_timer.active = true;
        dprintk("set next event, delta_ns = %d\n", delta_ns);
    }

    if (delta_ns == 0) {
        delta_ns = 1;
    }

    u32 freq = arm_timer.cntfrq;
    u64 cycles = ns_to_cycles(delta_ns, freq);
    arm_cntp_tval_write(cycles);
}

static void arch_timer_shutdown(void) {
    arm_timer.active = false;
    u32 ctl = arm_cntp_ctl_read();
    ctl &= ~1U; /* ENABLE = 0 */
    arm_cntp_ctl_write(ctl);

    if (arm_timer.virq >= 0) {
        irq_disable(arm_timer.virq);
    }
}

static u64 arch_timer_read_counter(void) {
    return arm_cntpct_read();
}

static struct clockevent_ops arm_timer_clockevent_ops = {
    .set_next_event = arch_timer_set_next_event,
    // .set_periodic = NULL,
    .shutdown = arch_timer_shutdown,
};

static int arm_arch_timer_of_init(struct device_node *np, struct device_node *parent) {
    int virq;
    unsigned int cpu;

    (void)parent;

    virq = of_irq_get(np, 0);
    if (virq < 0) {
        printk("get virq failed\n");
        return -1;
    }

    cpu = get_cpuid();
    if (irq_percpu_request(virq, cpu, arm_arch_timer_irq_handler, "arm_arch_timer", NULL) < 0) {
        printk("percpu request failed\n");
        return -1;
    }
    
    arm_timer.virq = virq;
    arm_timer.cntfrq = arm_cntfrq_read();
    dprintk("cntfrq = %xu Hz\n", arm_timer.cntfrq);

    if (clocksource_register("arm_arch_timer_clocksource", &arch_timer_read_counter, arm_timer.cntfrq, &arm_timer) < 0) {
        dprintk("register clocksource failed\n");
        return -1;
    }

    if (clockevent_register("arm_arch_timer_clockevent", &arm_timer_clockevent_ops, true) < 0) {
        dprintk("register clockevent failed\n");
        return -1;
    }

    return 0;
}

TIMERCHIP_DECLARE(arm_arch_timer, "arm,armv7-timer", arm_arch_timer_of_init);
