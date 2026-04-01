#include <os/device.h>
#include <os/bus.h>
#include <os/list.h>
#include <os/of.h>


int device_register(struct device *dev) {
    if (!dev) {
        return -1;
    }
    bus_add_device(dev);
    return device_attach(dev);
}

int device_unregister(struct device *dev) {
    if (!dev) {
        return -1;
    }
    bus_remove_device(dev);
    return 0;
}