#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/riscv.h>
#include <asm/cpu.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/cpu.h>
#include <os/timekeeping.h>
#include <os/timer_chip.h>

struct riscv64_timer_data {
    int virq;
    int clock_freq;
    bool active[MAX_CPUS];
};

static struct riscv64_timer_data riscv64_timer = {
    .virq = -1,
    .clock_freq = -1,
    .active = { false },
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
    int cpu = get_cpuid();
    u64 cycles;

    if (cpu < 0 || cpu >= MAX_CPUS) {
        cpu = 0;
    }

    if (!riscv64_timer.active[cpu]) {
        if (riscv64_timer.virq >= 0) {
            irq_enable(riscv64_timer.virq);
        }
        riscv64_timer.active[cpu] = true;
    }

    if (delta_ns == 0) {
        delta_ns = 1;
    }

    cycles = ns_to_cycles(delta_ns, riscv64_timer.clock_freq);
    if (cycles == 0) {
        cycles = 1;
    }

    stimecmp_w(time_r() + cycles);
}

static void riscv64_timer_shutdown(void)
{
    int cpu = get_cpuid();

    if (cpu < 0 || cpu >= MAX_CPUS) {
        cpu = 0;
    }

    riscv64_timer.active[cpu] = false;
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

    riscv64_timer.clock_freq = of_get_u32(np, "clock-frequency", -1);
    if (riscv64_timer.clock_freq <= 0) {
        return -1;
    }

    virq = riscv64_local_irq_map(CLINT_IRQ_TIMER);
    if (virq < 0) {
        return -1;
    }

    if (irq_request(virq, riscv64_timer_irq_handler, "riscv64_timer", NULL) < 0) {
        return -1;
    }

    riscv64_timer.virq = virq;
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        riscv64_timer.active[cpu] = false;
    }

    if (clocksource_register("riscv64_time_clocksource",
                             riscv64_timer_read_counter,
                             riscv64_timer.clock_freq,
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
