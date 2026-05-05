#include "os/types.h"
#include <os/fdt.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/bswap.h>
#include <os/device.h>
#include <os/of.h>

struct device_node* of_get_next_child(const struct device_node *node,struct device_node *prev)
{
	struct device_node *next;

	if (!node)
		return NULL;

	next = prev ? prev->sibling : node->children;
	for (; next; next = next->sibling)
		if (next)
			break;
	return next;
}

struct device_node *of_find_node_by_path(const char *path) {
    if (!fdt_root_node || !path)
        return NULL;
    struct device_node *current_node = (struct device_node *)fdt_root_node;
    char *path_dup = strdup(path);
    char *token = strtok(path_dup, "/");
 
    while (token) {
        current_node = find_child_node_by_name(current_node, token);
        if (!current_node) {
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return current_node;
}

// struct device_node *of_find_node_by_compatible(const char *compatible_prop) {
//     if (!fdt_root_node || !compatible_prop)
//         return NULL;

//     // 使用队列进行深度优先搜索
//     struct device_node **queue = (struct device_node **)kmalloc(sizeof(struct device_node *) * 512);
//     int front = 0, rear = 0;

//     queue[rear] = (struct device_node *)fdt_root_node;
//     rear++;

//     while (front < rear) {
//         struct device_node *current_node = queue[front];
//         front++;

//         // 检查当前节点的 compatible 属性
//         struct device_prop *prop = current_node->properties;
//         while (prop) {
//             if (strcmp(prop->name, "compatible") == 0) {
//                 if (strncmp((const char *)prop->value, compatible_prop, prop->length) == 0) {
//                     kfree(queue);
//                     return current_node;
//                 }
//             }
//             prop = prop->next;
//         }

//         // 将子节点加入队列
//         struct device_node *child = current_node->children;
//         while (child) {
//             queue[rear] = child;
//             rear++;
//             child = child->sibling;
//         }
//     }

//     kfree(queue);
//     return NULL;
// }

// 递归辅助函数：深度优先遍历子树，查找匹配 compatible 的节点
static struct device_node *
__of_find_node_by_compatible_recursive(struct device_node *node, const char *compatible)
{
    if (!node || !compatible)
        return NULL;

    // 1. 检查当前节点是否匹配
    struct device_prop *prop = node->properties;
    while (prop) {
        if (strcmp(prop->name, "compatible") == 0) {
            if (strncmp((const char *)prop->value, compatible, prop->length) == 0) {
                return node; // 找到匹配，直接返回
            }
        }
        prop = prop->next;
    }

    // 2. 递归遍历所有子节点
    struct device_node *child = node->children;
    while (child) {
        struct device_node *found = __of_find_node_by_compatible_recursive(child, compatible);
        if (found) {
            return found; // 子树中找到，向上返回
        }
        child = child->sibling;
    }

    // 3. 当前节点和所有子节点都不匹配
    return NULL;
}

struct device_node *of_find_node_by_compatible(const char *compatible_prop) {
    if (!fdt_root_node || !compatible_prop)
        return NULL;

    // 从根节点开始递归查找
    return __of_find_node_by_compatible_recursive(fdt_root_node, compatible_prop);
}

struct device_prop *of_get_property_by_name(const struct device_node *node, const char *name) {
    if (!node || !name)
        return NULL;

    struct device_prop *prop = node->properties;
    while (prop) {
        if (strcmp(prop->name, name) == 0) {
            return prop;
        }
        prop = prop->next;
    }
    return NULL;
}

int of_get_child_node_count(const struct device_node *node) {
    if (!node)
        return -1;

    int count = 0;
    struct device_node *child = node->children;
    while (child) {
        count++;
        child = child->sibling;
    }
    return count;
}

void *of_get_property(const struct device_node *node, const char *name, u32 *lenp) {
    struct device_prop *prop = of_get_property_by_name(node, name);
    if (prop) {
        if (lenp) {
            *lenp = prop->length;
        }
        return prop->value;
    }
    return NULL;
}

u32 fdt_get_phandle(const struct device_node *node) {
    if (!node)
        return 0;
    struct device_prop *prop = of_get_property_by_name(node, "phandle");
    if (prop) {
        return be32_to_cpu(*(u32 *)prop->value);
    }
    return 0;
}

u32 *of_get_reg(const struct device_node *node) {
    if (!node)
        return 0;
    struct device_prop *prop = of_get_property_by_name(node, "reg");
    if (prop) {
        return (u32 *)prop->value;
    }
    return 0;
}

u32 *of_read_u32_array(const struct device_node *node, const char *prop_name, int count) {
    if (!node || !prop_name || count <= 0)
        return NULL;

    struct device_prop *prop = of_get_property_by_name(node, prop_name);
    if (!prop || prop->length < count * sizeof(u32))
        return NULL;

    u32 *array = (u32 *)kmalloc(count * sizeof(u32));
    for (int i = 0; i < count; i++) {
        array[i] = be32_to_cpu(((u32 *)prop->value)[i]);
    }
    return array;
}

u64 *of_read_u64_array(const struct device_node *node, const char *prop_name, int count) {
    if (!node || !prop_name || count <= 0)
        return NULL;

    struct device_prop *prop = of_get_property_by_name(node, prop_name);
    if (!prop || prop->length < count * sizeof(u64))
        return NULL;

    u64 *array = (u64 *)kmalloc(count * sizeof(u64));
    for (int i = 0; i < count; i++) {
        array[i] = be64_to_cpu(((u64 *)prop->value)[i]);
    }
    return array;
}

struct device_node *of_find_node_by_phandle(u32 phandle) {
    if (!fdt_root_node)
        return NULL;

    if (phandle <= PHANDLE_MAX - 1) {
        return phandle_table[phandle];
    }

    int front = 0;
    int rear = 0;
    struct device_node **queue = (struct device_node **)kmalloc(sizeof(struct device_node*)*512);
    queue[rear] = (struct device_node *)fdt_root_node;
    rear++;

    while (front < rear) {
        struct device_node *curr = queue[front];
        front++;

        u32 curr_phandle = fdt_get_phandle(curr);
        if (phandle == curr_phandle) {
            return curr;
        }

        struct device_node *child = curr->children;
        while (child) {
            queue[rear] = child;
            rear++;
            child = child->sibling;
        }
    }
    kfree(queue);
    return NULL;
}

int of_get_address_cells(const struct device_node *node) {
    struct device_node *current = (struct device_node *)node->parent;
    struct device_prop *prop = NULL;

    while (current) {
        prop = of_get_property_by_name(current, "#address-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(__be32 *)prop->value);
        }
    }
    return 1;
}

int of_get_size_cells(const struct device_node *node) {
    if (!node)
        return -1;
    struct device_node *current = (struct device_node *)node->parent;
    struct device_prop *prop = NULL;

    while (current) {
        prop = of_get_property_by_name(current, "#size-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(__be32 *)prop->value);
        }
    }
    return 1;
}

struct device_node *of_get_interrupt_parent(const struct device_node *node) {
    if (!node)
        return NULL;
    struct device_prop *prop = of_get_property_by_name(node, "interrupt-parent");
    if (!prop) {
        return NULL;
    }

    u32 phandle = be32_to_cpu(*(u32 *)prop->value);
    return of_find_node_by_phandle(phandle);
}

int of_device_is_available(const struct device_node *node) {
    if (!node) {
        return -1;
    }

    struct device_prop *status = of_get_property_by_name(node, "status");

    if (!status) {
        return 0;
    }

    if (0 == strcmp(status->value, "okay")) {
       return 0; 
    }

    if (0 == strcmp(status->value, "disabled")) {
        return -1;
    }

    return -1;
}

struct device_node *of_find_all_nodes(struct device_node *prev) {
	struct device_node *np;
	if (!prev) {
		np = fdt_root_node;
	} else if (prev->children) {
		np = prev->children;
	} else {
		/* Walk back up looking for a sibling, or the end of the structure */
		np = prev;
		while (np->parent && !np->sibling)
			np = np->parent;
		np = np->sibling; /* Might be null at the end of the tree */
	}
	return np;
}

const struct of_device_id *of_match_node(const struct of_device_id *matches, const struct device_node *node) {
    if (!matches || !node)
        return NULL;

    for (const struct of_device_id *id = matches; id->compatible[0]; id++) {
        struct device_prop *prop = of_get_property_by_name(node, "compatible");
        if (!prop) continue;
        
        if (strcmp(prop->value, id->compatible) == 0) {
            return id;
        }
    }
    return NULL;
}


const struct of_device_id *of_match_device(const struct of_device_id *matches, const struct device *dev) {
    if (!matches || !dev || !dev->of_node)
        return NULL;

    return of_match_node(matches, dev->of_node);
}

struct device_node *of_find_matching_node_and_match(struct device_node *from,
					const struct of_device_id *matches,
					const struct of_device_id **match) {
	struct device_node *np;
	const struct of_device_id *m;

	if (match)
		*match = NULL;

	for_each_of_allnodes_from(from, np) {
		m = of_match_node(matches, np);
		if (m && np) {
			if (match)
				*match = m;
			break;
		}
	}

	return np;
}



void of_test() {
   
    // // fdt_walk_node(fdt_root_node, 0);
    // struct list_head list = LIST_HEAD_INIT(list);
    // fdt_walk(fdt_root_node, &list);
    // struct device_node *pos;
    // list_for_each_entry(pos, &list, struct device_node, node) {
    //     for (int i = 0; i < pos->depth; i++) {
    //         printk("  ");
    //     }
    //     printk("%s\n", pos->name);
    // }
    // struct device_node *node = of_find_node_by_path("/soc/rtc@0x50000000");
    // if (node) {
    //     printk("Found node: %s\n", node->name);
    // }
    // node = NULL;
    // node = of_find_node_by_compatible("wq,rtc");
    // if (node) {
    //     printk("Found node: %s\n", node->name);
    // }
    // // uintptr_t base, size;
    // // if (of_get_memory(&base, &size) == 0) {
    // //     printk("Memory base: %xu, size: %xu\n", base, size);
    // // }

    // node = of_find_node_by_compatible("wq,uart");
    // node = of_get_interrupt_parent(node);
    // if (node) {
    //     printk("interrupt-parent = %s\n", node->name);
    // }
    // // extern int of_scan_memory();
    // // extern void memblock_dump();
    // // of_scan_memory();
    // // memblock_dump();
}