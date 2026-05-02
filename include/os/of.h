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
#include <os/bswap.h>
#include <os/bitops.h>
#include <os/compiler_attributes.h>

/* flag descriptions */
#define OF_DYNAMIC	1 /* node and properties were allocated via kmalloc */
#define OF_DETACHED	2 /* node has been detached from the device tree */
#define OF_POPULATED	3 /* device already created for the node */
#define OF_POPULATED_BUS	4 /* of_platform_populate recursed to children of this node */

static inline void of_node_set_flag(struct device_node *n, unsigned long flag)
{
	n->_flags |= BIT(flag);
}

static inline void of_node_clear_flag(struct device_node *n, unsigned long flag)
{
	n->_flags &= ~BIT(flag);
}

#define MAX_PHANDLE_ARGS 16
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	u32 args[MAX_PHANDLE_ARGS];
};

extern struct device_node* of_get_next_child(const struct device_node *node,struct device_node *prev);
extern struct device_node* of_find_node_by_path(const char* path);
extern struct device_node* of_find_node_by_compatible(const char* compatible_prop);
extern struct device_prop* of_get_property_by_name(const struct device_node* node, const char* name);
extern void *of_get_property(const struct device_node *node, const char *name, u32 *lenp);
extern struct device_node* of_find_node_by_phandle(u32 phandle);
extern int of_get_child_node_count(const struct device_node *node);
extern int of_get_address_cells(const struct device_node *node);
extern int of_get_size_cells(const struct device_node *node);
extern u32* of_get_reg(const struct device_node *node);
extern u32 *of_read_u32_array(const struct device_node *node, const char *prop_name, int count);
extern u64 *of_read_u64_array(const struct device_node *node, const char *prop_name, int count);
extern struct device_node* of_get_interrupt_parent(const struct device_node *node);
extern int of_device_is_available(const struct device_node *node);
extern int of_device_is_type(const struct device_node *node, const char *type);
extern int of_scan_memory();
extern int of_scan_reserved_memory();
extern struct device *of_device_create(struct device_node *np, struct device *parent, struct bus_type *bus);
extern const struct of_device_id *of_match_node(const struct of_device_id *matches, const struct device_node *node);
extern struct device_node *of_find_all_nodes(struct device_node *prev);
extern struct device_node *of_find_matching_node_and_match(struct device_node *from,const struct of_device_id *matches,const struct of_device_id **match);
static inline struct device_node *of_find_matching_node(struct device_node *from, const struct of_device_id *matches) {
	return of_find_matching_node_and_match(from, matches, NULL);
}

#define for_each_child_of_node(parent, child) \
	for (child = of_get_next_child(parent, NULL); child != NULL; \
	     child = of_get_next_child(parent, child))

#define for_each_of_allnodes_from(from, dn) \
	for (dn = of_find_all_nodes(from); dn; dn = of_find_all_nodes(dn))

#define for_each_matching_node(dn, matches) \
	for (dn = of_find_matching_node(NULL, matches); dn; \
	     dn = of_find_matching_node(dn, matches))

static inline u64 of_read_number(const __be32 *cell, int size) {
	u64 r = 0;
	while (size--)
		r = (r << 32) | be32_to_cpu(*(cell++));
	return r;
}

#define _OF_DECLARE(table, name, compat, fn, fn_type)			\
	static const struct of_device_id __of_table_##name		\
		__used __section("__"#table"_of_table")			\
		 = { .compatible = compat,				\
		     .data = (fn == (fn_type)NULL) ? fn : fn  }

typedef int (*of_init_fn_2)(struct device_node *, struct device_node *);
typedef void (*of_init_fn_1)(struct device_node *);

#define OF_DECLARE_1(table, name, compat, fn) \
		_OF_DECLARE(table, name, compat, fn, of_init_fn_1)
#define OF_DECLARE_2(table, name, compat, fn) \
		_OF_DECLARE(table, name, compat, fn, of_init_fn_2)

extern void of_test();
#endif // __KERNEL_OF_H__