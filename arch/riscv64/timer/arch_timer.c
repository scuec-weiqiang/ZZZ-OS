/**
 * @FilePath: /ZZZ-OS/arch/riscv64/timer/arch_timer.c
 * @Description:
 */
#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/platform.h>
#include <asm/riscv.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/timekeeping.h>
#include <os/timer_chip.h>

struct riscv64_timer_data {
    bool active;
    int virq;
};

static struct riscv64_timer_data riscv64_timer = {
    .active = false,
    .virq = -1,
};

static u64 riscv64_timer_read_counter(void)
{
    return time_r();
}

static irqreturn_t riscv64_timer_irq_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    timekeeping_timer_interrupt();
    return IRQ_HANDLED;
}

static void riscv64_timer_set_next_event(u64 delta_ns)
{
    u64 cycles;

    if (!riscv64_timer.active) {
        if (riscv64_timer.virq >= 0) {
            irq_enable(riscv64_timer.virq);
        }
        riscv64_timer.active = true;
    }

    if (delta_ns == 0) {
        delta_ns = 1;
    }

    cycles = ns_to_cycles(delta_ns, SYS_CLOCK_FREQ);
    if (cycles == 0) {
        cycles = 1;
    }

    stimecmp_w(time_r() + cycles);
}

static void riscv64_timer_shutdown(void)
{
    riscv64_timer.active = false;
    if (riscv64_timer.virq >= 0) {
        irq_disable(riscv64_timer.virq);
    }
}

static struct clockevent_ops riscv64_timer_clockevent_ops = {
    .set_next_event = riscv64_timer_set_next_event,
    .shutdown = riscv64_timer_shutdown,
};

static int riscv64_timer_of_init(struct device_node *np, struct device_node *parent)
{
    int virq;

    (void)np;
    (void)parent;

    virq = riscv64_local_irq_map(CLINT_IRQ_TIMER);
    if (virq < 0) {
        return -1;
    }

    if (irq_request(virq, riscv64_timer_irq_handler, "riscv64_timer", NULL) < 0) {
        return -1;
    }

    riscv64_timer.virq = virq;

    if (clocksource_register("riscv64_time_clocksource",
                             riscv64_timer_read_counter,
                             SYS_CLOCK_FREQ,
                             &riscv64_timer) < 0) {
        return -1;
    }

    if (clockevent_register("riscv64_time_clockevent",
                            &riscv64_timer_clockevent_ops,
                            true) < 0) {
        return -1;
    }

    return 0;
}

TIMERCHIP_DECLARE(riscv64_timer, "wq,time", riscv64_timer_of_init);
