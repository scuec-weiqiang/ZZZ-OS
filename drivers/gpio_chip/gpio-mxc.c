/*
 * MXC GPIO support. (c) 2008 Daniel Mack <daniel@caiaq.de>
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 *
 * Based on code from Freescale,
 * Copyright (C) 2004-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

 #include <os/err.h>
 #include <os/init.h>
 #include <os/io.h>
 #include <os/irq.h>

 #include <os/platform_device.h>
 #include <os/basic_mmio_gpio.h>
 #include <os/of.h>
 #include <os/devicetable.h>
 #include <os/module.h>
 #include <os/kmalloc.h>

 #include "gpiolib.h"

 
 enum mxc_gpio_hwtype {
	 IMX1_GPIO,	/* runs on i.mx1 */
	 IMX21_GPIO,	/* runs on i.mx21 and i.mx27 */
	 IMX31_GPIO,	/* runs on i.mx31 */
	 IMX35_GPIO,	/* runs on all other i.mx */
 };
 
 /* device type dependent stuff */
 struct mxc_gpio_hwdata {
	 unsigned dr_reg;
	 unsigned gdir_reg;
	 unsigned psr_reg;
	 unsigned icr1_reg;
	 unsigned icr2_reg;
	 unsigned imr_reg;
	 unsigned isr_reg;
	 int edge_sel_reg;
	 unsigned low_level;
	 unsigned high_level;
	 unsigned rise_edge;
	 unsigned fall_edge;
 };
 
 struct mxc_gpio_port {
	 struct list_head node;
	 void __iomem *base;
	 int irq;
	 int irq_high;
	 struct irq_domain *domain;
	 struct bgpio_chip bgc;
	 u32 both_edges;
 };
 
 static struct mxc_gpio_hwdata imx1_imx21_gpio_hwdata = {
	 .dr_reg		= 0x1c,
	 .gdir_reg	= 0x00,
	 .psr_reg	= 0x24,
	 .icr1_reg	= 0x28,
	 .icr2_reg	= 0x2c,
	 .imr_reg	= 0x30,
	 .isr_reg	= 0x34,
	 .edge_sel_reg	= -EINVAL,
	 .low_level	= 0x03,
	 .high_level	= 0x02,
	 .rise_edge	= 0x00,
	 .fall_edge	= 0x01,
 };
 
 static struct mxc_gpio_hwdata imx31_gpio_hwdata = {
	 .dr_reg		= 0x00,
	 .gdir_reg	= 0x04,
	 .psr_reg	= 0x08,
	 .icr1_reg	= 0x0c,
	 .icr2_reg	= 0x10,
	 .imr_reg	= 0x14,
	 .isr_reg	= 0x18,
	 .edge_sel_reg	= -EINVAL,
	 .low_level	= 0x00,
	 .high_level	= 0x01,
	 .rise_edge	= 0x02,
	 .fall_edge	= 0x03,
 };
 
 static struct mxc_gpio_hwdata imx35_gpio_hwdata = {
	 .dr_reg		= 0x00,
	 .gdir_reg	= 0x04,
	 .psr_reg	= 0x08,
	 .icr1_reg	= 0x0c,
	 .icr2_reg	= 0x10,
	 .imr_reg	= 0x14,
	 .isr_reg	= 0x18,
	 .edge_sel_reg	= 0x1c,
	 .low_level	= 0x00,
	 .high_level	= 0x01,
	 .rise_edge	= 0x02,
	 .fall_edge	= 0x03,
 };
 
 static enum mxc_gpio_hwtype mxc_gpio_hwtype;
 static struct mxc_gpio_hwdata *mxc_gpio_hwdata;
 
 #define GPIO_DR			(mxc_gpio_hwdata->dr_reg)
 #define GPIO_GDIR		(mxc_gpio_hwdata->gdir_reg)
 #define GPIO_PSR		(mxc_gpio_hwdata->psr_reg)
 #define GPIO_ICR1		(mxc_gpio_hwdata->icr1_reg)
 #define GPIO_ICR2		(mxc_gpio_hwdata->icr2_reg)
 #define GPIO_IMR		(mxc_gpio_hwdata->imr_reg)
 #define GPIO_ISR		(mxc_gpio_hwdata->isr_reg)
 #define GPIO_EDGE_SEL		(mxc_gpio_hwdata->edge_sel_reg)
 
 #define GPIO_INT_LOW_LEV	(mxc_gpio_hwdata->low_level)
 #define GPIO_INT_HIGH_LEV	(mxc_gpio_hwdata->high_level)
 #define GPIO_INT_RISE_EDGE	(mxc_gpio_hwdata->rise_edge)
 #define GPIO_INT_FALL_EDGE	(mxc_gpio_hwdata->fall_edge)
 #define GPIO_INT_BOTH_EDGES	0x4
 
 static struct platform_device_id mxc_gpio_devtype[] = {
	 {
		 .name = "imx1-gpio",
		 .driver_data = IMX1_GPIO,
	 }, {
		 .name = "imx21-gpio",
		 .driver_data = IMX21_GPIO,
	 }, {
		 .name = "imx31-gpio",
		 .driver_data = IMX31_GPIO,
	 }, {
		 .name = "imx35-gpio",
		 .driver_data = IMX35_GPIO,
	 }, {
		 /* sentinel */
	 }
 };
 
 static const struct of_device_id mxc_gpio_dt_ids[] = {
	 { .compatible = "fsl,imx1-gpio", .data = &mxc_gpio_devtype[IMX1_GPIO], },
	 { .compatible = "fsl,imx21-gpio", .data = &mxc_gpio_devtype[IMX21_GPIO], },
	 { .compatible = "fsl,imx31-gpio", .data = &mxc_gpio_devtype[IMX31_GPIO], },
	 { .compatible = "fsl,imx35-gpio", .data = &mxc_gpio_devtype[IMX35_GPIO], },
	 { /* sentinel */ }
 };
 
 /*
  * MX2 has one interrupt *for all* gpio ports. The list is used
  * to save the references to all ports, so that mx2_gpio_irq_handler
  * can walk through all interrupt status registers.
  */
 static LIST_HEAD(mxc_gpio_ports);
 
 /* Note: This driver assumes 32 GPIOs are handled in one register */
 
 static void mxc_flip_edge(struct mxc_gpio_port *port, u32 gpio)
 {
	 void __iomem *reg = port->base;
	 u32 bit, val;
	 int edge;
 
	 reg += GPIO_ICR1 + ((gpio & 0x10) >> 2); /* lower or upper register */
	 bit = gpio & 0xf;
	 val = readl(reg);
	 edge = (val >> (bit << 1)) & 3;
	 val &= ~(0x3 << (bit << 1));
	 if (edge == GPIO_INT_HIGH_LEV) {
		 edge = GPIO_INT_LOW_LEV;
		//  pr_debug("mxc: switch GPIO %d to low trigger\n", gpio);
	 } else if (edge == GPIO_INT_LOW_LEV) {
		 edge = GPIO_INT_HIGH_LEV;
		//  pr_debug("mxc: switch GPIO %d to high trigger\n", gpio);
	 } else {
		//  pr_err("mxc: invalid configuration for GPIO %d: %x\n",
				// gpio, edge);
		 return;
	 }
	 writel(val | (edge << (bit << 1)), reg);
 }
 
 
static void mxc_gpio_get_hw(struct platform_device *pdev){
	 const struct of_device_id *of_id =
			 of_match_device(mxc_gpio_dt_ids, &pdev->dev);
	 enum mxc_gpio_hwtype hwtype;
 
	 if (of_id)
		 pdev->id_entry = of_id->data;
	 hwtype = pdev->id_entry->driver_data;
 
	 if (mxc_gpio_hwtype) {
		 /*
		  * The driver works with a reasonable presupposition,
		  * that is all gpio ports must be the same type when
		  * running on one soc.
		  */
		 return;
	 }
 
	 if (hwtype == IMX35_GPIO) {
		mxc_gpio_hwdata = &imx35_gpio_hwdata;
		// dprintk("hwtype = imx35\n");
	 }
		 
	 else if (hwtype == IMX31_GPIO)
		 mxc_gpio_hwdata = &imx31_gpio_hwdata;
	 else
		 mxc_gpio_hwdata = &imx1_imx21_gpio_hwdata;
 
	 mxc_gpio_hwtype = hwtype;
 }
 
static int mxc_gpio_probe(struct platform_device *pdev) {

	struct device_node *np = pdev->dev.of_node;
	dprintk("mxc_gpio_probe called\n");
	if (of_get_property_by_name(np, "gpio-controller") == NULL) {
		return -EINVAL;
	}

	struct mxc_gpio_port *port;
	int err;

	mxc_gpio_get_hw(pdev);

	port = kzalloc(sizeof(*port));
	if (!port)
		return -ENOMEM;

	port->base = (void*)platform_ioremap_resource(pdev, 0);
	if (IS_ERR(port->base)) {
		return PTR_ERR(port->base);
	} else {
		dprintk("gpio chip base:0x%x\n",port->base);
	}
		

	 /* disable the interrupt and clear the status */
	writel(0, port->base + GPIO_IMR);
	writel(~0, port->base + GPIO_ISR);

	err = bgpio_init(&port->bgc, &pdev->dev, 4,
			port->base + GPIO_PSR,
			port->base + GPIO_DR, NULL,
			port->base + GPIO_GDIR, NULL, 0);
	if (err)
		goto out_bgio;

	port->bgc.gc.base =  pdev->id * 32;

	err = gpiochip_add(&port->bgc.gc);
	if (err)
		goto out_bgpio_remove;

	list_add_tail(&mxc_gpio_ports, &port->node);
dprintk("gpio-mxc init success\n");
	return 0;
 
 out_bgpio_remove:
	 bgpio_remove(&port->bgc);
 out_bgio:
	 dprintk("%s failed with errno %d\n",err);
	 return err;
}
 
static struct platform_driver mxc_gpio_driver = {
	 .driver		= {
		 .name	= "gpio-mxc",
		 .of_match_table = mxc_gpio_dt_ids,
	 },
	 .probe		= mxc_gpio_probe,
	 .id_table	= mxc_gpio_devtype,
};
 
static int  gpio_mxc_init(void){
	 return platform_driver_register(&mxc_gpio_driver);
}
core_initcall(gpio_mxc_init);

//  postcore_initcall(gpio_mxc_init);
 
//  MODULE_AUTHOR("Freescale Semiconductor, "
// 		   "Daniel Mack <danielncaiaq.de>, "
// 		   "Juergen Beisert <kernel@pengutronix.de>");
//  MODULE_DESCRIPTION("Freescale MXC GPIO");
//  MODULE_LICENSE("GPL");
 