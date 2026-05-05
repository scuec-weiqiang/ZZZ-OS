/*
 * Generic driver for memory-mapped GPIO controllers.
 *
 * Copyright 2008 MontaVista Software, Inc.
 * Copyright 2008,2010 Anton Vorontsov <cbouatmailru@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * ....``.```~~~~````.`.`.`.`.```````'',,,.........`````......`.......
 * ...``                                                         ```````..
 * ..The simplest form of a GPIO controller that the driver supports is``
 *  `.just a single "data" register, where GPIO state can be read and/or `
 *    `,..written. ,,..``~~~~ .....``.`.`.~~.```.`.........``````.```````
 *        `````````
                                    ___
_/~~|___/~|   . ```~~~~~~       ___/___\___     ,~.`.`.`.`````.~~...,,,,...
__________|~$@~~~        %~    /o*o*o*o*o*o\   .. Implementing such a GPIO .
o        `                     ~~~~\___/~~~~    ` controller in FPGA is ,.`
                                                 `....trivial..'~`.```.```
 *                                                    ```````
 *  .```````~~~~`..`.``.``.
 * .  The driver supports  `...       ,..```.`~~~```````````````....````.``,,
 * .   big-endian notation, just`.  .. A bit more sophisticated controllers ,
 *  . register the device with -be`. .with a pair of set/clear-bit registers ,
 *   `.. suffix.  ```~~`````....`.`   . affecting the data register and the .`
 *     ``.`.``...```                  ```.. output pins are also supported.`
 *                        ^^             `````.`````````.,``~``~``~~``````
 *                                                   .                  ^^
 *   ,..`.`.`...````````````......`.`.`.`.`.`..`.`.`..
 * .. The expectation is that in at least some cases .    ,-~~~-,
 *  .this will be used with roll-your-own ASIC/FPGA .`     \   /
 *  .logic in Verilog or VHDL. ~~~`````````..`````~~`       \ /
 *  ..````````......```````````                             \o_
 *                                                           |
 *                              ^^                          / \
 *
 *           ...`````~~`.....``.`..........``````.`.``.```........``.
 *            `  8, 16, 32 and 64 bits registers are supported, and``.
 *            . the number of GPIOs is determined by the width of   ~
 *             .. the registers. ,............```.`.`..`.`.~~~.`.`.`~
 *               `.......````.```
 */

#include <os/init.h>
#include <os/err.h>
#include <os/module.h>
#include <os/spinlock.h>
#include <os/compiler_attributes.h>
#include <os/utils.h>
#include <os/io.h>
#include <os/platform_device.h>
#include <os/bitops.h>
#include <os/basic_mmio_gpio.h>

#include "gpiolib.h"

static void bgpio_write8(void __iomem *reg, unsigned long data) {
	writeb(data, reg);
}

static unsigned long bgpio_read8(void __iomem *reg) {
	return readb(reg);
}

static void bgpio_write16(void __iomem *reg, unsigned long data) {
	writew(data, reg);
}

static unsigned long bgpio_read16(void __iomem *reg)
{
	return readw(reg);
}

static void bgpio_write32(void __iomem *reg, unsigned long data)
{
	writel(data, reg);
}

static unsigned long bgpio_read32(void __iomem *reg)
{
	return readl(reg);
}

static unsigned long bgpio_pin2mask(struct bgpio_chip *bgc, unsigned int pin)
{
	return 1 << pin;
}

static int bgpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	return !!(bgc->read_reg(bgc->reg_dat) & bgc->pin2mask(bgc, gpio));
}

static void bgpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long mask = bgc->pin2mask(bgc, gpio);
	unsigned long flags;

	flags = spin_lock_irqsave(&bgc->lock);

	if (val)
		bgc->data |= mask;
	else
		bgc->data &= ~mask;

	bgc->write_reg(bgc->reg_dat, bgc->data);

	spin_unlock_irqrestore(&bgc->lock, flags);
}

static void bgpio_set_with_clear(struct gpio_chip *gc, unsigned int gpio,
				 int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long mask = bgc->pin2mask(bgc, gpio);

	if (val)
		bgc->write_reg(bgc->reg_set, mask);
	else
		bgc->write_reg(bgc->reg_clr, mask);
}

static void bgpio_set_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long mask = bgc->pin2mask(bgc, gpio);
	unsigned long flags;

	flags = spin_lock_irqsave(&bgc->lock);

	if (val)
		bgc->data |= mask;
	else
		bgc->data &= ~mask;

	bgc->write_reg(bgc->reg_set, bgc->data);

	spin_unlock_irqrestore(&bgc->lock, flags);
}

static void bgpio_multiple_get_masks(struct bgpio_chip *bgc,
				     unsigned long *mask, unsigned long *bits,
				     unsigned long *set_mask,
				     unsigned long *clear_mask)
{
	int i;

	*set_mask = 0;
	*clear_mask = 0;

	for (i = 0; i < bgc->bits; i++) {
		if (*mask == 0)
			break;
		if (__test_and_clear_bit(i, mask)) {
			if (test_bit(i, bits))
				*set_mask |= bgc->pin2mask(bgc, i);
			else
				*clear_mask |= bgc->pin2mask(bgc, i);
		}
	}
}

static void bgpio_set_multiple_single_reg(struct bgpio_chip *bgc,
					  unsigned long *mask,
					  unsigned long *bits,
					  void __iomem *reg)
{
	unsigned long flags;
	unsigned long set_mask, clear_mask;

	flags = spin_lock_irqsave(&bgc->lock);

	bgpio_multiple_get_masks(bgc, mask, bits, &set_mask, &clear_mask);

	bgc->data |= set_mask;
	bgc->data &= ~clear_mask;

	bgc->write_reg(reg, bgc->data);

	spin_unlock_irqrestore(&bgc->lock, flags);
}

static void bgpio_set_multiple(struct gpio_chip *gc, unsigned long *mask,
			       unsigned long *bits)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	bgpio_set_multiple_single_reg(bgc, mask, bits, bgc->reg_dat);
}

static void bgpio_set_multiple_set(struct gpio_chip *gc, unsigned long *mask,
				   unsigned long *bits)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	bgpio_set_multiple_single_reg(bgc, mask, bits, bgc->reg_set);
}

static void bgpio_set_multiple_with_clear(struct gpio_chip *gc,
					  unsigned long *mask,
					  unsigned long *bits)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long set_mask, clear_mask;

	bgpio_multiple_get_masks(bgc, mask, bits, &set_mask, &clear_mask);

	if (set_mask)
		bgc->write_reg(bgc->reg_set, set_mask);
	if (clear_mask)
		bgc->write_reg(bgc->reg_clr, clear_mask);
}

static int bgpio_simple_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	return 0;
}

static int bgpio_simple_dir_out(struct gpio_chip *gc, unsigned int gpio,
				int val)
{
	gc->set(gc, gpio, val);

	return 0;
}

static int bgpio_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long flags;

	flags = spin_lock_irqsave(&bgc->lock);

	bgc->dir &= ~bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	spin_unlock_irqrestore(&bgc->lock, flags);

	return 0;
}

static int bgpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long flags;

	gc->set(gc, gpio, val);

	flags = spin_lock_irqsave(&bgc->lock);

	bgc->dir |= bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	spin_unlock_irqrestore(&bgc->lock, flags);

	return 0;
}

static int bgpio_dir_in_inv(struct gpio_chip *gc, unsigned int gpio)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long flags;

	flags = spin_lock_irqsave(&bgc->lock);

	bgc->dir |= bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	spin_unlock_irqrestore(&bgc->lock, flags);

	return 0;
}

static int bgpio_dir_out_inv(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);
	unsigned long flags;

	gc->set(gc, gpio, val);

	flags = spin_lock_irqsave(&bgc->lock);

	bgc->dir &= ~bgc->pin2mask(bgc, gpio);
	bgc->write_reg(bgc->reg_dir, bgc->dir);

	spin_unlock_irqrestore(&bgc->lock, flags);

	return 0;
}

static int bgpio_simple_xlate(struct gpio_chip *chip, const struct of_phandle_args *args, u32 *offset, u32 *flags)
{
	if (args->args_count < 2)
		return -EINVAL;

	*offset = args->args[0];
	*flags  = args->args[1];
	
	return 0;
}

static int bgpio_setup_accessors(struct device *dev, struct bgpio_chip *bgc)
{

	switch (bgc->bits) {
		case 8:
			bgc->read_reg	= bgpio_read8;
			bgc->write_reg	= bgpio_write8;
			break;
		case 16:
			bgc->read_reg	= bgpio_read16;
			bgc->write_reg	= bgpio_write16;
			break;
		case 32:
			bgc->read_reg	= bgpio_read32;
			bgc->write_reg	= bgpio_write32;
			break;
		default:
			dprintk("unsupported data width %u bits\n", bgc->bits);
			return -EINVAL;
	}

	bgc->pin2mask = bgpio_pin2mask;

	return 0;
}

/*
 * Create the device and allocate the resources.  For setting GPIO's there are
 * three supported configurations:
 *
 *	- single input/output register resource (named "dat").
 *	- set/clear pair (named "set" and "clr").
 *	- single output register resource and single input resource ("set" and
 *	dat").
 *
 * For the single output register, this drives a 1 by setting a bit and a zero
 * by clearing a bit.  For the set clr pair, this drives a 1 by setting a bit
 * in the set register and clears it by setting a bit in the clear register.
 * The configuration is detected by which resources are present.
 *
 * For setting the GPIO direction, there are three supported configurations:
 *
 *	- simple bidirection GPIO that requires no configuration.
 *	- an output direction register (named "dirout") where a 1 bit
 *	indicates the GPIO is an output.
 *	- an input direction register (named "dirin") where a 1 bit indicates
 *	the GPIO is an input.
 */
static int bgpio_setup_io(struct bgpio_chip *bgc,
			  void __iomem *dat,
			  void __iomem *set,
			  void __iomem *clr)
{

	bgc->reg_dat = dat;
	if (!bgc->reg_dat)
		return -EINVAL;

	if (set && clr) {
		bgc->reg_set = set;
		bgc->reg_clr = clr;
		bgc->gc.set = bgpio_set_with_clear;
		bgc->gc.set_multiple = bgpio_set_multiple_with_clear;
	} else if (set && !clr) {
		bgc->reg_set = set;
		bgc->gc.set = bgpio_set_set;
		bgc->gc.set_multiple = bgpio_set_multiple_set;
	} else {
		bgc->gc.set = bgpio_set;
		bgc->gc.set_multiple = bgpio_set_multiple;
	}

	bgc->gc.get = bgpio_get;

	return 0;
}

static int bgpio_setup_direction(struct bgpio_chip *bgc,
				 void __iomem *dirout,
				 void __iomem *dirin)
{
	if (dirout && dirin) {
		return -EINVAL;
	} else if (dirout) {
		bgc->reg_dir = dirout;
		bgc->gc.direction_output = bgpio_dir_out;
		bgc->gc.direction_input = bgpio_dir_in;
	} else if (dirin) {
		bgc->reg_dir = dirin;
		bgc->gc.direction_output = bgpio_dir_out_inv;
		bgc->gc.direction_input = bgpio_dir_in_inv;
	} else {
		bgc->gc.direction_output = bgpio_simple_dir_out;
		bgc->gc.direction_input = bgpio_simple_dir_in;
	}

	return 0;
}

static int bgpio_request(struct gpio_chip *chip, unsigned gpio_pin)
{
	if (gpio_pin < chip->ngpio)
		return 0;

	return -EINVAL;
}

int bgpio_remove(struct bgpio_chip *bgc)
{
	gpiochip_remove(&bgc->gc);
	return 0;
}


int bgpio_init(struct bgpio_chip *bgc, struct device *dev,
	       unsigned long sz, void __iomem *dat, void __iomem *set,
	       void __iomem *clr, void __iomem *dirout, void __iomem *dirin,
	       unsigned long flags)
{
	int ret;

	if (!is_power_of_two(sz))
		return -EINVAL;

	bgc->bits = sz * 8;
	if (bgc->bits > BITS_PER_LONG)
		return -EINVAL;

	spin_lock_init(&bgc->lock);
	bgc->gc.dev = dev;
	bgc->gc.label = dev_name(dev);
	bgc->gc.base = -1;
	bgc->gc.ngpio = bgc->bits;
	bgc->gc.request = bgpio_request;


	bgc->gc.of_xlate = bgpio_simple_xlate;

	ret = bgpio_setup_io(bgc, dat, set, clr);
	if (ret)
		return ret;

	ret = bgpio_setup_accessors(dev, bgc);
	if (ret)
		return ret;

	ret = bgpio_setup_direction(bgc, dirout, dirin);
	if (ret)
		return ret;

	bgc->data = bgc->read_reg(bgc->reg_dat);
	if (bgc->gc.set == bgpio_set_set &&
			!(flags & BGPIOF_UNREADABLE_REG_SET))
		bgc->data = bgc->read_reg(bgc->reg_set);
	if (bgc->reg_dir && !(flags & BGPIOF_UNREADABLE_REG_DIR))
		bgc->dir = bgc->read_reg(bgc->reg_dir);

	return ret;
}


#ifdef CONFIG_GPIO_GENERIC_PLATFORM

static void __iomem *bgpio_map(struct platform_device *pdev,
			       const char *name,
			       resource_size_t sane_sz,
			       int *err)
{
	struct device *dev = &pdev->dev;
	struct resource *r;
	resource_size_t start;
	resource_size_t sz;
	void __iomem *ret;

	*err = 0;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!r)
		return NULL;

	sz = resource_size(r);
	if (sz != sane_sz) {
		*err = -EINVAL;
		return NULL;
	}

	start = r->start;
	if (!devm_request_mem_region(dev, start, sz, r->name)) {
		*err = -EBUSY;
		return NULL;
	}

	ret = devm_ioremap(dev, start, sz);
	if (!ret) {
		*err = -ENOMEM;
		return NULL;
	}

	return ret;
}

static int bgpio_pdev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *r;
	void __iomem *dat;
	void __iomem *set;
	void __iomem *clr;
	void __iomem *dirout;
	void __iomem *dirin;
	unsigned long sz;
	unsigned long flags = pdev->id_entry->driver_data;
	int err;
	struct bgpio_chip *bgc;
	struct bgpio_pdata *pdata = dev_get_platdata(dev);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dat");
	if (!r)
		return -EINVAL;

	sz = resource_size(r);

	dat = bgpio_map(pdev, "dat", sz, &err);
	if (!dat)
		return err ? err : -EINVAL;

	set = bgpio_map(pdev, "set", sz, &err);
	if (err)
		return err;

	clr = bgpio_map(pdev, "clr", sz, &err);
	if (err)
		return err;

	dirout = bgpio_map(pdev, "dirout", sz, &err);
	if (err)
		return err;

	dirin = bgpio_map(pdev, "dirin", sz, &err);
	if (err)
		return err;

	bgc = devm_kzalloc(&pdev->dev, sizeof(*bgc), GFP_KERNEL);
	if (!bgc)
		return -ENOMEM;

	err = bgpio_init(bgc, dev, sz, dat, set, clr, dirout, dirin, flags);
	if (err)
		return err;

	if (pdata) {
		if (pdata->label)
			bgc->gc.label = pdata->label;
		bgc->gc.base = pdata->base;
		if (pdata->ngpio > 0)
			bgc->gc.ngpio = pdata->ngpio;
	}

	platform_set_drvdata(pdev, bgc);

	return gpiochip_add(&bgc->gc);
}

static int bgpio_pdev_remove(struct platform_device *pdev)
{
	struct bgpio_chip *bgc = platform_get_drvdata(pdev);

	return bgpio_remove(bgc);
}

static const struct platform_device_id bgpio_id_table[] = {
	{
		.name		= "basic-mmio-gpio",
		.driver_data	= 0,
	}, {
		.name		= "basic-mmio-gpio-be",
		.driver_data	= BGPIOF_BIG_ENDIAN,
	},
	{ }
};
MODULE_DEVICE_TABLE(platform, bgpio_id_table);

static struct platform_driver bgpio_driver = {
	.driver = {
		.name = "basic-mmio-gpio",
	},
	.id_table = bgpio_id_table,
	.probe = bgpio_pdev_probe,
	.remove = bgpio_pdev_remove,
};

module_platform_driver(bgpio_driver);

#endif /* CONFIG_GPIO_GENERIC_PLATFORM */

