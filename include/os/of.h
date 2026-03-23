/**
 * @FilePath: /ZZZ-OS/include/os/of.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-16 23:53:35
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_OF_H__
#define __KERNEL_OF_H__

#include <os/types.h>
#include <os/fdt.h>
#include <os/device.h>

extern struct device_node* of_get_next_child(const struct device_node *node,struct device_node *prev);
extern struct device_node* of_find_node_by_path(const char* path);
extern struct device_node* of_find_node_by_compatible(const char* compatible_prop);
extern struct device_prop* of_get_prop_by_name(const struct device_node* node, const char* name);
extern struct device_node* of_find_node_by_phandle(uint32_t phandle);
extern uint32_t of_get_address_cells(const struct device_node *node);
extern uint32_t of_get_size_cells(const struct device_node *node);
extern uint32_t* of_get_reg(const struct device_node *node);
extern uint32_t *of_read_u32_array(const struct device_node *node, const char *prop_name, int count);
extern uint64_t *of_read_u64_array(const struct device_node *node, const char *prop_name, int count);
extern struct device_node* of_get_interrupt_parent(const struct device_node *node);
extern int of_device_is_available(const struct device_node *node);
extern int of_device_is_type(const struct device_node *node, const char *type);
extern int of_scan_memory();
extern int of_scan_reserved_memory();
extern struct device *of_device_create(struct device_node *np, struct device *parent, struct bus_type *bus);
extern const struct of_device_id *of_match_node(const struct of_device_id *matches, const struct device_node *node);


#define for_each_child_of_node(parent, child) \
	for (child = of_get_next_child(parent, NULL); child != NULL; \
	     child = of_get_next_child(parent, child))
         
extern void of_test();
#endif // __KERNEL_OF_H__