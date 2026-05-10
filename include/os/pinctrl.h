#ifndef __OS_PINCTRL_H
#define __OS_PINCTRL_H

#include <os/device.h>
#include <os/list.h>
#include <os/of.h>

struct pinctrl_device;

struct pinctrl_ops {
    int (*apply_dt_node)(struct pinctrl_device *pctldev,
                         struct device_node *cfg_np);
};

struct pinctrl_device {
    const char *name;
    struct device *dev;
    struct device_node *of_node;
    const struct pinctrl_ops *ops;
    void *driver_data;
    struct list_head node;
};

int pinctrl_register(struct pinctrl_device *pctldev);
int pinctrl_select_state(struct device *dev, const char *state_name);

static inline void pinctrl_set_drvdata(struct pinctrl_device *pctldev, void *data)
{
    pctldev->driver_data = data;
}

static inline void *pinctrl_get_drvdata(const struct pinctrl_device *pctldev)
{
    return pctldev->driver_data;
}

#endif
