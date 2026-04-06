/**
 * @FilePath: /ZZZ-OS/arch/arm/include/asm/arch_timer.h
 * @Description:
 */
#ifndef __ARM_ARCH_TIMER_H__
#define __ARM_ARCH_TIMER_H__

#include <os/types.h>

enum arch_timer_hz {
    SYS_HZ_1000 = 1000,
    SYS_HZ_250 = 250,
    SYS_HZ_100 = 100,
    SYS_HZ_50 = 50,
    SYS_HZ_20 = 20,
    SYS_HZ_10 = 10,
    SYS_HZ_5 = 5,
    SYS_HZ_2 = 2,
    SYS_HZ_1 = 1,
};

extern void arch_timer_init(enum arch_timer_hz hz);
extern void arch_timer_start(void);
extern void arch_timer_pause(void);
extern void arch_timer_period(enum arch_timer_hz hz);
extern void arch_timer_reload(void);
extern uint64_t systick(void);
extern void systick_up(void);
extern uint64_t arch_timer_counter(void);
extern uint32_t arch_timer_frequency(void);

#endif
