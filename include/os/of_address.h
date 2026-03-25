#ifndef OS_OF_ADDRESS_H
#define OS_OF_ADDRESS_H

#include <os/resource.h>
#include <os/device.h>

extern int of_address_to_resource(struct device_node *dev, int index, struct resource *res);

#endif // OS_OF_PLATFORM_H