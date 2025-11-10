#ifndef __KERNEL_OF_H__
#define __KERNEL_OF_H__

#include <os/types.h>
#include <drivers/of/fdt.h>

extern struct device_node* of_find_node_by_path(const char* path);
extern struct device_node* of_find_node_by_compatible(const char* compatible_prop);
extern struct device_prop* of_get_prop_by_name(const struct device_node* node, const char* name);
extern struct device_node* of_find_node_by_phandle(uint32_t phandle);
extern uint32_t of_get_address_cells(const struct device_node *node);
extern uint32_t of_get_size_cells(const struct device_node *node);
extern uint32_t* of_get_reg(const struct device_node *node);
extern int of_get_memory(uintptr_t *base, uintptr_t *size);
extern struct device_node* of_get_interrupt_parent(const struct device_node *node);
extern int of_device_is_available(const struct device_node *node);
#endif // __KERNEL_OF_H__