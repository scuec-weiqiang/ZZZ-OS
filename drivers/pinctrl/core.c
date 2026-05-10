#include <os/pinctrl.h>

#include <os/bswap.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>

static struct list_head pinctrl_devices = LIST_HEAD_INIT(pinctrl_devices);

static struct pinctrl_device *of_find_pinctrl_by_node(struct device_node *np)
{
    struct pinctrl_device *pctldev;

    list_for_each_entry(pctldev, &pinctrl_devices, struct pinctrl_device, node) {
        if (pctldev->of_node == np) {
            return pctldev;
        }
    }

    return NULL;
}

static int of_property_match_string(const struct device_node *np,
                                    const char *prop_name,
                                    const char *match)
{
    u32 len = 0;
    const char *prop;
    u32 offset = 0;
    int index = 0;

    prop = of_get_property(np, prop_name, &len);
    if (!prop || !match) {
        return -ENOENT;
    }

    while (offset < len) {
        size_t slen = strlen(prop + offset);

        if (offset + slen >= len) {
            break;
        }

        if (strcmp(prop + offset, match) == 0) {
            return index;
        }

        offset += slen + 1;
        index++;
    }

    return -ENOENT;
}

static int pinctrl_get_state_index(const struct device_node *np, const char *state_name)
{
    int index;
    u32 len = 0;

    index = of_property_match_string(np, "pinctrl-names", state_name);
    if (index >= 0) {
        return index;
    }

    if (strcmp(state_name, "default") == 0 &&
        of_get_property(np, "pinctrl-0", &len) != NULL) {
        return 0;
    }

    return index;
}

static int pinctrl_apply_cfg_node(struct device *dev, struct device_node *cfg_np)
{
    struct device_node *ctrl_np;
    struct pinctrl_device *pctldev;

    if (!cfg_np || !cfg_np->parent) {
        return -EINVAL;
    }

    ctrl_np = cfg_np->parent;
    pctldev = of_find_pinctrl_by_node(ctrl_np);
    if (!pctldev) {
        printk("pinctrl: controller for %s is not registered\n", cfg_np->full_path);
        return -ENODEV;
    }

    if (!pctldev->ops || !pctldev->ops->apply_dt_node) {
        printk("pinctrl: controller %s does not support DT configs\n", pctldev->name);
        return -EINVAL;
    }

    return pctldev->ops->apply_dt_node(pctldev, cfg_np);
}

int pinctrl_register(struct pinctrl_device *pctldev)
{
    if (!pctldev || !pctldev->of_node || !pctldev->ops ||
        !pctldev->ops->apply_dt_node) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&pctldev->node);
    list_add(&pinctrl_devices, &pctldev->node);
    return 0;
}

int pinctrl_select_state(struct device *dev, const char *state_name)
{
    char prop_name[32];
    const __be32 *phandles;
    u32 len = 0;
    int state_index;
    int count;
    int i;

    if (!dev || !dev->of_node || !state_name) {
        return -EINVAL;
    }

    state_index = pinctrl_get_state_index(dev->of_node, state_name);
    if (state_index < 0) {
        return state_index;
    }

    snprintk(prop_name, sizeof(prop_name), "pinctrl-%d", state_index);
    phandles = of_get_property(dev->of_node, prop_name, &len);
    if (!phandles || len < sizeof(u32)) {
        return -ENOENT;
    }

    count = len / sizeof(u32);
    for (i = 0; i < count; i++) {
        u32 phandle = be32_to_cpu(phandles[i]);
        struct device_node *cfg_np;
        int ret;

        if (!phandle) {
            continue;
        }

        cfg_np = of_find_node_by_phandle(phandle);
        if (!cfg_np) {
            printk("pinctrl: invalid phandle 0x%x for %s\n",
                   phandle, dev_name(dev));
            return -ENODEV;
        }

        ret = pinctrl_apply_cfg_node(dev, cfg_np);
        if (ret < 0) {
            printk("pinctrl: failed to apply %s for %s, ret=%d\n",
                   cfg_np->full_path, dev_name(dev), ret);
            return ret;
        }
    }

    return 0;
}
