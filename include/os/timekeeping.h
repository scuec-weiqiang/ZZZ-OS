#ifndef __OS_TIMEKEEPING_H
#define __OS_TIMEKEEPING_H

#include <os/types.h>
#include <os/utils.h>

struct clocksource {
    char name[32];
    u64 (*read_counter)(void);
    u32 freq_hz;
    void *clocksource_data;
};
#define NSEC_PER_SEC 1000000000ULL

static inline u64 ns_to_cycles(u64 ns, u32 freq) {
    return div_u64(ns * freq, NSEC_PER_SEC);
}

static inline u64 cycles_to_ns(u64 cycles, u32 freq) {
    return div_u64(cycles * NSEC_PER_SEC, freq);
}

extern int clocksource_register(char *name, u64 (*read_counter)(void), u32 freq_hz, void* data);
extern struct clocksource *sys_clocksource(void);

struct clockevent_ops {
    void (*set_next_event)(u64 delta_ns);
    void (*shutdown)(void);
};

struct clockevent {
    char name[32];
    struct clockevent_ops *ops;
    bool oneshot;
};

extern int clockevent_register(char *name, struct clockevent_ops *ops, bool oneshot);
extern struct clockevent *sys_clockevent(void);


extern u64 monotonic_ns(void);
extern void time_init(void);
extern void program_next_event(u64 next, u64 now);
extern void timekeeping_timer_interrupt(void);


typedef struct timespec 
{
    u64 tv_sec;   // 秒数（自 Unix 纪元时间 1970-01-01 00:00:00 起）
    long tv_nsec;  // 纳秒数（0 ~ 999,999,999）
}timespec_t;

#endif