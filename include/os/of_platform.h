#ifndef OS_OF_PLATFORM_H
#define OS_OF_PLATFORM_H
#include <os/fdt.h>
#include <os/devicetable.h>
#include <os/device.h>

extern const struct of_device_id of_default_bus_match_table[];
extern void of_platform_populate(struct device_node *root, const struct of_device_id *matches, struct device *parent);

#endif // OS_OF_PLATFORM_H