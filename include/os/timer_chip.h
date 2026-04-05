/**
 * @FilePath: /ZZZ-OS/include/os/timer_chip.h
 * @Description:
 */
#ifndef __KERNEL_TIMER_CHIP_H__
#define __KERNEL_TIMER_CHIP_H__

#include <os/of.h>

typedef int (*timerchip_init_cb_t)(struct device_node *, struct device_node *);

extern void timer_chip_init(void);

#define TIMERCHIP_DECLARE(name, compat, fn) OF_DECLARE_2(timerchip, name, compat, fn)

#endif
