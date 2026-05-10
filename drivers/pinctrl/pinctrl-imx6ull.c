#include <os/pinctrl.h>

#include <os/bswap.h>
#include <os/err.h>
#include <os/init.h>
#include <os/io.h>
#include <os/kmalloc.h>
#include <os/of.h>
#include <os/platform_device.h>
#include <os/printk.h>

#define IMX_NO_PAD_CTL 0x80000000U
#define IMX_PAD_SION   0x40000000U
#define IMX_PAD_MASK   (~(IMX_NO_PAD_CTL | IMX_PAD_SION))
#define IMX_MUX_SION   BIT(4)
#define IMX_FSL_PIN_SIZE 6

struct imx6ull_pinctrl {
    void *base;
    struct pinctrl_device pctldev;
};

static int imx6ull_pinctrl_apply_pin(struct imx6ull_pinctrl *ipctl,
                                     const u32 *pin_data)
{
    u32 mux_reg = pin_data[0];
    u32 conf_reg = pin_data[1];
    u32 input_reg = pin_data[2];
    u32 mux_mode = pin_data[3];
    u32 input_val = pin_data[4];
    u32 config = pin_data[5];
    u32 mux_val = mux_mode;

    if (config & IMX_PAD_SION) {
        mux_val |= IMX_MUX_SION;
    }

    writel(mux_val, ipctl->base + mux_reg);

    if (input_reg) {
        writel(input_val, ipctl->base + input_reg);
    }

    if (!(config & IMX_NO_PAD_CTL) && conf_reg) {
        writel(config & IMX_PAD_MASK, ipctl->base + conf_reg);
    }

    return 0;
}

static int imx6ull_pinctrl_apply_dt_node(struct pinctrl_device *pctldev,
                                         struct device_node *cfg_np)
{
    struct imx6ull_pinctrl *ipctl = pinctrl_get_drvdata(pctldev);
    const __be32 *prop;
    u32 len = 0;
    int cells;
    int i;

    prop = of_get_property(cfg_np, "fsl,pins", &len);
    if (!prop || len < IMX_FSL_PIN_SIZE * sizeof(u32)) {
        printk("imx6ull-pinctrl: missing fsl,pins in %s\n", cfg_np->full_path);
        return -EINVAL;
    }

    cells = len / sizeof(u32);
    if (cells % IMX_FSL_PIN_SIZE != 0) {
        printk("imx6ull-pinctrl: invalid fsl,pins length in %s\n", cfg_np->full_path);
        return -EINVAL;
    }

    for (i = 0; i < cells; i += IMX_FSL_PIN_SIZE) {
        u32 pin_data[IMX_FSL_PIN_SIZE];
        int j;

        for (j = 0; j < IMX_FSL_PIN_SIZE; j++) {
            pin_data[j] = be32_to_cpu(prop[i + j]);
        }

        imx6ull_pinctrl_apply_pin(ipctl, pin_data);
    }

    return 0;
}

static const struct pinctrl_ops imx6ull_pinctrl_ops = {
    .apply_dt_node = imx6ull_pinctrl_apply_dt_node,
};

static int imx6ull_pinctrl_probe(struct platform_device *pdev)
{
    struct imx6ull_pinctrl *ipctl;
    int ret;

    ipctl = kzalloc(sizeof(*ipctl));
    if (!ipctl) {
        return -ENOMEM;
    }

    ipctl->base = (void *)platform_ioremap_resource(pdev, 0);
    if (!ipctl->base) {
        kfree(ipctl);
        return -ENODEV;
    }

    ipctl->pctldev.name = dev_name(&pdev->dev);
    ipctl->pctldev.dev = &pdev->dev;
    ipctl->pctldev.of_node = pdev->dev.of_node;
    ipctl->pctldev.ops = &imx6ull_pinctrl_ops;
    pinctrl_set_drvdata(&ipctl->pctldev, ipctl);

    ret = pinctrl_register(&ipctl->pctldev);
    if (ret < 0) {
        kfree(ipctl);
        return ret;
    }

    platform_set_drvdata(pdev, ipctl);
    return 0;
}

static int imx6ull_pinctrl_remove(struct platform_device *pdev)
{
    struct imx6ull_pinctrl *ipctl = platform_get_drvdata(pdev);

    if (ipctl) {
        kfree(ipctl);
    }

    return 0;
}

static const struct of_device_id imx6ull_pinctrl_dt_ids[] = {
    { .compatible = "fsl,imx6ul-iomuxc", },
    { .compatible = "fsl,imx6ull-iomuxc", },
    {},
};

static struct platform_driver imx6ull_pinctrl_driver = {
    .probe = imx6ull_pinctrl_probe,
    .remove = imx6ull_pinctrl_remove,
    .driver = {
        .name = "imx6ull-pinctrl",
        .of_match_table = imx6ull_pinctrl_dt_ids,
    },
};

static int imx6ull_pinctrl_init(void)
{
    return platform_driver_register(&imx6ull_pinctrl_driver);
}

core_initcall(imx6ull_pinctrl_init);
