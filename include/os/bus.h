#ifndef __KERNEL_BUS_H
#define __KERNEL_BUS_H
#include <os/device.h>
#include <os/list.h>

extern struct list_head global_bus_list;

extern int bus_register(struct bus_type *bt); 
extern int bus_add_device(struct device *dev);
extern int bus_remove_device(struct device *dev);
extern int bus_add_driver(struct device_driver *drv);
extern int bus_remove_driver(struct device_driver *drv); 
extern struct bus_type* bus_get_by_name(const char *name); 

#endif