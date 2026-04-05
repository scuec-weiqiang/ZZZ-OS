/**
 * @FilePath     : /ZZZ-OS/drivers/of/of_irq.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 14:25:46
 * @LastEditTime : 2026-03-26 17:54:23
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/of.h>
#include <os/device.h>
#include <os/resource.h>
#include <os/printk.h>
#include <os/kmalloc.h>
#include <os/of_irq.h>
#include <os/irq_domain.h>

static int __of_irq_parse_one(struct device_node *np, int index, struct of_phandle_args *out) {
    uint32_t size = 0;
    __be32 *cells;
    uint32_t offset = 0;
    int current = 0;

    if (!np || !out || index < 0) {
        return -1;
    }

    cells = (__be32 *)of_get_property(np, "interrupts-extended", &size);
    if (!cells || size == 0) {
        return -1;
    }

    while (offset < size / sizeof(__be32)) {
        uint32_t phandle;
        struct device_node *intc;
        void *prop;
        uint32_t prop_size = 0;
        uint32_t irq_cells;

        phandle = be32_to_cpu(cells[offset]);
        intc = of_find_node_by_phandle(phandle);
        if (!intc) {
            return -1;
        }

        prop = of_get_property(intc, "#interrupt-cells", &prop_size);
        if (!prop || prop_size < sizeof(__be32)) {
            return -1;
        }

        irq_cells = be32_to_cpu(*(__be32 *)prop);
        // printk("irq_cells=%d\n", irq_cells);
        if (irq_cells > MAX_PHANDLE_ARGS) {
            return -1;
        }
        if (offset + 1 + irq_cells > size / sizeof(__be32)) {
            return -1;
        }

        if (current == index) {
            out->np = intc;
            out->args_count = irq_cells;
            for (uint32_t i = 0; i < irq_cells; i++) {
                out->args[i] = be32_to_cpu(cells[offset + 1 + i]);
                // printk("  - arg[%d] = %d\n", i, out->args[i]);
            }
            return 0;
        }

        offset += 1 + irq_cells;
        current++;
    }

    return -1;
}

int of_irq_count(struct device_node *np) {
    uint32_t size = 0;
    __be32 *cells;
    uint32_t offset = 0;
    int count = 0;

    if (!np) {
        return -1;
    }

    cells = (__be32 *)of_get_property(np, "interrupts-extended", &size);
    // printk("of_irq_count: node=%s, interrupts-extended size=%d\n", np->name, size / sizeof(__be32));
    if (!cells || size == 0) {
        return 0;
    }

    while (offset < size / sizeof(__be32)) {
        uint32_t phandle;
        struct device_node *intc;
        void *prop;
        uint32_t prop_size = 0;
        uint32_t irq_cells;

        phandle = be32_to_cpu(cells[offset]);
        intc = of_find_node_by_phandle(phandle);
        if (!intc) {
            return -1;
        }

        prop = of_get_property(intc, "#interrupt-cells", &prop_size);
        if (!prop || prop_size < sizeof(__be32)) {
            return -1;
        }

        irq_cells = be32_to_cpu(*(__be32 *)prop);
        if (offset + 1 + irq_cells > size / sizeof(__be32)) {
            return -1;
        }

        offset += 1 + irq_cells;
        count++;
    }

    return count;
}

int of_irq_parse_one(struct device_node *np, int index, struct of_phandle_args *out) {
    return __of_irq_parse_one(np, index, out);
}


int of_irq_get(struct device_node *np, int index) {
    struct of_phandle_args args;
    struct irq_domain *domain;
    struct irq_chip *chip;

    if (of_irq_parse_one(np, index, &args) < 0) {
        return -1;
    }
    domain = irq_find_host(args.np);
    if (!domain) {
        return -1;
    }
    chip = irq_chip_lookup(args.np);
    if (!chip) {
        return -1;
    }
    irq_set_hwirq_and_chip(domain, args.args[0], chip);
    return irq_domain_add_mapping(domain, args.args[0]);
}

int of_irq_to_resource(struct device_node *np, int index, struct resource *res) {
    struct of_phandle_args args;
    if (of_irq_parse_one(np, index, &args) < 0) {
        return -1;
    }

    res->start = 0;
    res->irq = args.args[0];
    res->flags = IORESOURCE_IRQ;

    return 0;
}   

void of_irq_init(struct of_device_id *matches) {
    struct device_node *np;
    int ret = 0; 

    for_each_matching_node(np, matches) {
        // printk("of_irq_init: node %s\n",np->full_path);
        
        if (!of_get_property_by_name(np, "interrupt-controller")) {
            // printk("no property interrupt-controller\n");
            continue;
        }
        
        const struct of_device_id *match;
        match = of_match_node(matches,np);

        of_irq_init_cb_t cb= (of_irq_init_cb_t)match->data;
        ret = cb(np,NULL);
        if (ret) {
            printk("%s :irq chip init failed\n", np->full_path);
            continue;
        }
    }
    return ;
}