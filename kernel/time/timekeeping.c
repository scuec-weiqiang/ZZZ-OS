/**
 * @FilePath: /ZZZ-OS/kernel/timekeeping.c
 * @Description: minimal timekeeping based on arch timer + optional RTC boot time
 */
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/timekeeping.h>
#include <os/utils.h>
#include <os/timer_chip.h>
#include <os/timerqueue.h>

static struct clocksource *__clocksource;

static void set_clocksource(struct clocksource *cs) {
    if (!__clocksource) {
        __clocksource = cs;
    } 
}

int clocksource_register(char *name, uint64_t (*read_counter)(void), uint32_t freq_hz, void* data) {
    if (name == NULL || read_counter == NULL || freq_hz == 0) {
        return -1;
    }

    struct clocksource *cs = (struct clocksource *)kmalloc(sizeof(struct clocksource));
    if (!cs) {
        return -1;
    }

    strncpy(cs->name, name, sizeof(cs->name) - 1);
    cs->read_counter = read_counter;
    cs->freq_hz = freq_hz;
    cs->clocksource_data = data;
    set_clocksource(cs);

    return 0;
}

struct clocksource *sys_clocksource(void) {
    return __clocksource;
}

static struct clockevent *__clockevent;
static void set_clockevent(struct clockevent *ce) {
    if (!__clockevent) {
        __clockevent = ce;
    }
}

int clockevent_register(char *name, struct clockevent_ops *ops, bool oneshot) {
    if (name == NULL || ops == NULL || ops->set_next_event == NULL || ops->shutdown == NULL) {
        return -1;
    }

    struct clockevent *ce = (struct clockevent *)kmalloc(sizeof(struct clockevent));
    if (!ce) {
        return -1;
    }

    strncpy(ce->name, name, sizeof(ce->name) - 1);
    ce->ops = ops;
    ce->oneshot = oneshot;

    set_clockevent(ce);

    return 0;
}

struct clockevent *sys_clockevent(void) {
    return __clockevent;
}

uint64_t monotonic_ns(void) {
    uint64_t cnt;
    uint32_t freq;
    uint64_t sec;
    uint64_t rem_ns;
    uint32_t rem;

    if (sys_clocksource()->read_counter == NULL || sys_clocksource()->freq_hz == 0) {
        return 0;
    }

    cnt = sys_clocksource()->read_counter();
    freq = sys_clocksource()->freq_hz;
    if (freq == 0) {
        return 0;
    }

    sec = divmod_u64(cnt, freq, &rem);
    rem_ns = div_u64((uint64_t)rem * NSEC_PER_SEC, freq);
    return sec * NSEC_PER_SEC + rem_ns;
}

void program_next_event(uint64_t next, uint64_t now) {
    if (sys_clockevent() && sys_clockevent()->ops->set_next_event) {
        uint64_t delta_ns = next > now ? next - now : 0;
        sys_clockevent()->ops->set_next_event(delta_ns);
    }
}

void timekeeping_timer_interrupt(void)
{
    uint64_t now = monotonic_ns();
    dprintk("timer interrupt at %duns\n", now);
    timerqueue_run_expired(now);

    uint64_t next = timerqueue_next_deadline();
    // dprintk("next timer deadline at %dns\n", next);
    
    program_next_event(next, now);
}

static void default_timer_callback(struct timer *t, void *arg) {
    
}


void time_init(void) {
    timer_chip_init();
    timerqueue_init();
    dprintk("time int success\n");
}