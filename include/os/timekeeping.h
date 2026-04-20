#ifndef __OS_TIMEKEEPING_H
#define __OS_TIMEKEEPING_H

#include <os/types.h>
#include <os/utils.h>

struct clocksource {
    char name[32];
    uint64_t (*read_counter)(void);
    uint32_t freq_hz;
    void *clocksource_data;
};
#define NSEC_PER_SEC 1000000000ULL

static inline uint64_t ns_to_cycles(uint64_t ns, uint32_t freq) {
    return div_u64(ns * freq, NSEC_PER_SEC);
}

static inline uint64_t cycles_to_ns(uint64_t cycles, uint32_t freq) {
    return div_u64(cycles * NSEC_PER_SEC, freq);
}

extern int clocksource_register(char *name, uint64_t (*read_counter)(void), uint32_t freq_hz, void* data);
extern struct clocksource *sys_clocksource(void);

struct clockevent_ops {
    void (*set_next_event)(uint64_t delta_ns);
    void (*shutdown)(void);
};

struct clockevent {
    char name[32];
    struct clockevent_ops *ops;
    bool oneshot;
};

extern int clockevent_register(char *name, struct clockevent_ops *ops, bool oneshot);
extern struct clockevent *sys_clockevent(void);


extern uint64_t monotonic_ns(void);
extern void time_init(void);
extern void program_next_event(uint64_t next, uint64_t now);
extern void timekeeping_timer_interrupt(void);


typedef struct timespec 
{
    uint64_t tv_sec;   // 秒数（自 Unix 纪元时间 1970-01-01 00:00:00 起）
    uint64_t tv_nsec;  // 纳秒数（0 ~ 999,999,999）
}timespec_t;

#endif