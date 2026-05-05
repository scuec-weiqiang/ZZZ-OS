/*
 * Internal GPIO functions.
 *
 * Copyright (C) 2013, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* modified from linux */

#ifndef GPIOLIB_H
#define GPIOLIB_H

#include <os/err.h>
#include <os/device.h>
#include <os/gpio/driver.h>

enum of_gpio_flags;

struct gpio_desc *of_get_named_gpiod_flags(struct device_node *np,
		   const char *list_name, int index, enum of_gpio_flags *flags);

struct gpio_desc *gpiochip_get_desc(struct gpio_chip *chip, u16 hwnum);

extern struct spinlock gpio_lock;
extern struct list_head gpio_chips;

struct gpio_desc {
    struct gpio_chip *chip;
    unsigned int offset;
    unsigned long flags;
    const char *label;
    int requested;

	#define GPIO_ACTIVE_HIGH   0
	#define GPIO_ACTIVE_LOW    (1 << 0)
	#define GPIO_OPEN_DRAIN    (1 << 1)
	#define GPIO_OPEN_SOURCE   (1 << 2)
};

int gpiod_request(struct gpio_desc *desc, const char *label);
void gpiod_free(struct gpio_desc *desc);

/*
 * Return the GPIO number of the passed descriptor relative to its chip
 */
static int __maybe_unused gpio_chip_hwgpio(const struct gpio_desc *desc) {
	return desc - &desc->chip->desc[0];
}

int gpiochip_add(struct gpio_chip *chip);
void gpiochip_remove(struct gpio_chip *chip);
struct gpio_chip *of_find_gpiochip_by_node(struct device_node *np);


#endif /* GPIOLIB_H */
