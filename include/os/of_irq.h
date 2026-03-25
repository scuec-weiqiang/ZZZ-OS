/**
 * @FilePath     : /ZZZ-OS/include/os/of_irq.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 17:53:13
 * @LastEditTime : 2026-03-25 18:08:17
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef OS_OF_IRQ_H
#define OS_OF_IRQ_H

#include <os/device.h>
#include <os/of.h>
#include <os/resource.h>

extern int of_irq_count(struct device_node *np);
extern int of_irq_parse_one(struct device_node *np, int index, struct of_phandle_args *out);
extern int of_irq_to_resource(struct device_node *np, int index, struct resource *res);

#endif
