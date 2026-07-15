#ifndef __OS_DEVNODE_H
#define __OS_DEVNODE_H

#include <os/types.h>
#include <fs/types.h>

struct device;

int devtmpfs_create_node(struct device *dev);
void devtmpfs_remove_node(struct device *dev);
int devtmpfs_mount(const char *path);

#endif
