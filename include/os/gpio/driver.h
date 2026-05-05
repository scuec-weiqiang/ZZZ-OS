#ifndef __OS_GPIO_DRIVER_H
#define __OS_GPIO_DRIVER_H

#include <os/device.h>
#include <os/spinlock.h>
#include <os/of.h>

struct gpio_desc;

struct gpio_chip {
    struct device *dev;
    const char *label;

    struct gpio_desc *desc;
    unsigned int base;
    unsigned int ngpio;

    spinlock_t lock;

    struct list_head list;
    void *priv;

    int (*of_xlate)(struct gpio_chip *chip,
        const struct of_phandle_args *gpiospec,
        u32 *offset,
        u32 *flags);

    int (*request)(struct gpio_chip *chip, unsigned int offset);
    void (*free)(struct gpio_chip *chip, unsigned int offset);

    void (*set_multiple)(struct gpio_chip *chip, unsigned long *mask, unsigned long *bits);

    int (*direction_input)(struct gpio_chip *chip, unsigned int offset);
    int (*direction_output)(struct gpio_chip *chip, unsigned int offset, int value);

    int (*get)(struct gpio_chip *chip, unsigned int offset);
    void (*set)(struct gpio_chip *chip, unsigned int offset, int value);

    int (*to_irq)(struct gpio_chip *chip, unsigned int offset); // 后面做
};

#endif