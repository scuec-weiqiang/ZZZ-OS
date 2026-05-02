/**
 * @FilePath     : /ZZZ-OS/drivers/of/of_address.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 12:01:01
 * @LastEditTime : 2026-03-25 20:50:31
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/device.h>
#include <os/of.h>
#include <os/resource.h>
#include <os/bswap.h>
#include <os/printk.h>
#include <os/mm.h>

/* Callbacks for bus specific translators */
struct of_bus {
	const char	*name;
	const char	*addresses;
	int	(*match)(struct device_node *parent);
	void (*count_cells)(struct device_node *child, int *addrc, int *sizec);
	// u64 (*map)(__be32 *addr, const __be32 *range, int na, int ns, int pna);
	// int		(*translate)(__be32 *addr, u64 offset, int na);
	unsigned int	(*get_flags)(const addr_t *addr);
};

static void of_bus_default_count_cells(struct device_node *dev, int *addrc, int *sizec)
{
	if (addrc)
		*addrc = of_get_address_cells(dev);
	if (sizec)
		*sizec = of_get_size_cells(dev);
}

static unsigned int of_bus_default_get_flags(const __be32 *addr)
{
	return IORESOURCE_MEM;
}

static struct of_bus of_busses[] = {
    {
		.name = "default",
		.addresses = "reg",
		.match = NULL,
		.count_cells = of_bus_default_count_cells,
		.get_flags = of_bus_default_get_flags,
	},
};


const __be32* of_address_to_resource(struct device_node *dev, int index, struct resource *res) {
    struct of_bus *bus = NULL;
    const __be32 *prop;
    unsigned int psize;
    int onesize, i, na, ns;

    for (int i = 0; i < sizeof(of_busses) / sizeof(of_busses[0]); i++) {
        if (!of_busses[i].match || of_busses[i].match(dev->parent)) {
            bus = &of_busses[i];
            break;
        }
    }

    if (!bus) {
        return NULL;
    }

    // printk("of_address_to_resource: bus=%s\n", bus->name);
    bus->count_cells(dev, &na, &ns);
    // printk("of_address_to_resource: na=%d, ns=%d\n", na, ns);

	prop = of_get_property(dev, bus->addresses, &psize);

	if (prop == NULL) {
        return NULL;
    }
	psize /= 4;

    onesize = na + ns;
	for (i = 0; psize >= onesize; psize -= onesize, prop += onesize, i++)
		if (i == index) {
            res->start = of_read_number(prop, na);
            res->size = of_read_number(prop + na, ns);
            res->flags = bus->get_flags(prop);
			return prop;
		}
	return NULL;
}


void *of_iomap(struct device_node *dev, int index) {
	struct resource res;
	if (!of_address_to_resource(dev, index, &res)) {
		return NULL;
	}
	printk("of_iomap: start=%xu, size=%xu\n", res.start, res.size);
	return ioremap(res.start, res.size);
}

