#include <os/device.h>
#include <os/list.h>
#include <os/string.h>

LIST_HEAD(global_bus_list);

int bus_register(struct bus_type *bt) {
    if (!bt || !bt->name || bt->match ) {
        return -1;
    }
    INIT_LIST_HEAD(&bt->node);
    list_add(&global_bus_list, &bt->node);

    INIT_LIST_HEAD(&bt->devices);
    INIT_LIST_HEAD(&bt->drivers);

    return 0;
}

int bus_add_device(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
    list_add(&dev->bus->devices, &dev->node);
    return 0;
}

int bus_remove_device(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
    list_del(&dev->node);
    return 0;
}

int bus_add_driver(struct device_driver *drv) {
    if (!drv || !drv->bus) {
        return -1;
    }
    list_add(&drv->bus->drivers, &drv->node);
    return 0;
}

int bus_remove_driver(struct device_driver *drv) {
    if (!drv || !drv->bus) {
        return -1;
    }
    list_del(&drv->node);
    return 0;
}

struct bus_type* bus_get_by_name(const char *name) {
    if (!name) {
        return NULL;
    }

    struct bus_type *bt;
    list_for_each_entry(bt, &global_bus_list, struct bus_type, node) {
        if (strcmp(bt->name, name) == 0) {
            return bt;  
        }
    }

    return NULL;

}