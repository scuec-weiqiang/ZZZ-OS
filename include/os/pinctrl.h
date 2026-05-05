#ifndef __OS_PINCTRL_H
#define __OS_PINCTRL_H

#include <os/device.h>

struct pinctrl_config {
    unsigned int pin;
    unsigned int function;
    unsigned int pull;
    unsigned int drive_strength;
    unsigned int flags;
};

struct pinctrl_state {
    const char *name;
    struct pinctrl_config *configs;
    unsigned int num_configs;
};

struct pinctrl_device {
    const char *name;
    struct pinctrl_state *states;
    unsigned int num_states;

    int (*set_mux)(struct pinctrl_device *pctldev,
                   unsigned int pin,
                   unsigned int function);

    int (*set_config)(struct pinctrl_device *pctldev,
                      unsigned int pin,
                      const struct pinctrl_config *cfg);
};


int pinctrl_register(struct pinctrl_device *pctldev);
int pinctrl_select_state(struct device *dev, const char *state_name);


#endif