#include "os/types.h"
#include <drivers/of/fdt.h>
#include <os/printk.h>
#include <os/string.h>
#include <fs/path.h>
#include <os/malloc.h>
#include <os/bswap.h>

struct device_node *of_find_node_by_path(const char *path) {
    if (!fdt_root_node || !path)
        return NULL;
    struct device_node *current_node = (struct device_node *)fdt_root_node;
    char *path_dup = strdup(path);
    char *token = path_split(path_dup, "/");
 
    while (token) {
        current_node = find_child_node_by_name(current_node, token);
        if (!current_node) {
            return NULL;
        }
        token = path_split(NULL, "/");
    }
    return current_node;
}

struct device_node *of_find_node_by_compatible(const char *compatible_prop) {
    if (!fdt_root_node || !compatible_prop)
        return NULL;

    // 使用队列进行深度优先搜索
    struct device_node **queue = (struct device_node **)malloc(sizeof(struct device_node *) * 512);
    int front = 0, rear = 0;

    queue[rear] = (struct device_node *)fdt_root_node;
    rear++;

    while (front < rear) {
        struct device_node *current_node = queue[front];
        front++;

        // 检查当前节点的 compatible 属性
        struct device_prop *prop = current_node->properties;
        while (prop) {
            if (strcmp(prop->name, "compatible") == 0) {
                if (strncmp((const char *)prop->value, compatible_prop, prop->length) == 0) {
                    free(queue);
                    return current_node;
                }
            }
            prop = prop->next;
        }

        // 将子节点加入队列
        struct device_node *child = current_node->children;
        while (child) {
            queue[rear] = child;
            rear++;
            child = child->sibling;
        }
    }

    free(queue);
    return NULL;
}

struct device_prop *of_get_prop_by_name(const struct device_node *node, const char *name) {
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

uint32_t fdt_get_phandle(const struct device_node *node) {
    if (!node)
        return 0;
    struct device_prop *prop = of_get_prop_by_name(node, "phandle");
    if (prop) {
        return be32_to_cpu(*(uint32_t *)prop->value);
    }
    return 0;
}

uint32_t *of_get_reg(const struct device_node *node) {
    if (!node)
        return 0;
    struct device_prop *prop = of_get_prop_by_name(node, "reg");
    if (prop) {
        return (uint32_t *)prop->value;
    }
    return 0;
}

uint32_t *of_read_u32_array(const struct device_node *node, const char *prop_name, int count) {
    if (!node || !prop_name || count <= 0)
        return NULL;

    struct device_prop *prop = of_get_prop_by_name(node, prop_name);
    if (!prop || prop->length < count * sizeof(uint32_t))
        return NULL;

    uint32_t *array = (uint32_t *)malloc(count * sizeof(uint32_t));
    for (int i = 0; i < count; i++) {
        array[i] = be32_to_cpu(((uint32_t *)prop->value)[i]);
    }
    return array;
}

uint64_t *of_read_u64_array(const struct device_node *node, const char *prop_name, int count) {
    if (!node || !prop_name || count <= 0)
        return NULL;

    struct device_prop *prop = of_get_prop_by_name(node, prop_name);
    if (!prop || prop->length < count * sizeof(uint64_t))
        return NULL;

    uint64_t *array = (uint64_t *)malloc(count * sizeof(uint64_t));
    for (int i = 0; i < count; i++) {
        array[i] = be64_to_cpu(((uint64_t *)prop->value)[i]);
    }
    return array;
}

struct device_node *of_find_node_by_phandle(uint32_t phandle) {
    if (!fdt_root_node)
        return NULL;

    if (phandle <= PHANDLE_MAX - 1) {
        return phandle_table[phandle];
    }

    int front = 0;
    int rear = 0;
    struct device_node **queue = (struct device_node **)malloc(sizeof(struct device_node*)*512);
    queue[rear] = (struct device_node *)fdt_root_node;
    rear++;

    while (front < rear) {
        struct device_node *curr = queue[front];
        front++;

        uint32_t curr_phandle = fdt_get_phandle(curr);
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
    free(queue);
    return NULL;
}

uint32_t of_get_address_cells(const struct device_node *node) {
    if (!node)
        return -1;
    struct device_node *current = (struct device_node *)node->parent;
    struct device_prop *prop = NULL;

    while (current) {
        prop = of_get_prop_by_name(current, "#address-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(uint32_t *)prop->value);
        }
    }
    return 2;
}

uint32_t of_get_size_cells(const struct device_node *node) {
    if (!node)
        return -1;
    struct device_node *current = (struct device_node *)node->parent;
    struct device_prop *prop = NULL;

    while (current) {
        prop = of_get_prop_by_name(current, "#size-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(uint32_t *)prop->value);
        }
    }
    return 2;
}


struct device_node *of_get_interrupt_parent(const struct device_node *node) {
    if (!node)
        return NULL;
    struct device_prop *prop = of_get_prop_by_name(node, "interrupt-parent");
    if (!prop) {
        return NULL;
    }

    uint32_t phandle = be32_to_cpu(*(uint32_t *)prop->value);
    return of_find_node_by_phandle(phandle);
}

int of_device_is_available(const struct device_node *node) {
    if (!node) {
        return -1;
    }

    struct device_prop *status = of_get_prop_by_name(node, "status");

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

int of_device_is_type(const struct device_node *node, const char *type) {
    if (!node || !type)
        return -1;

    if (strcmp(type, "soc") == 0 && strcmp(node->name, "soc") == 0) {
       return 0; 
    }

    struct device_prop *prop = of_get_prop_by_name(node, "device_type");
    if (!prop)
        return -1;

    if (strncmp((const char *)prop->value, type, prop->length) == 0) {
        return 0;
    }

    prop = of_get_prop_by_name(node, "compatible");
    if (!prop)
        return -1;
    if (strncmp((const char *)prop->value, type, prop->length) == 0) {
        return 0;
    }
    
    return -1;
}

// int of_node_is_bus(const struct device_node *np)
// {
//     if (of_node_is_type(np, "simple-bus"))
//         return 0;
//     if (fdt_get_prop_by_name(np, "ranges"))
//         return 0;
//     // if (fdt_get_prop_by_name(np, "compatible") &&
//     //     strstr((char*)prop->value, "bus"))
//     //     return 1;
//     return 0;
// }

void of_test() {
   
    // fdt_walk_node(fdt_root_node, 0);
    struct list_head list = LIST_HEAD_INIT(list);
    fdt_walk(fdt_root_node, &list);
    struct device_node *pos;
    list_for_each_entry(pos, &list, struct device_node, node) {
        for (int i = 0; i < pos->depth; i++) {
            printk("  ");
        }
        printk("%s\n", pos->name);
    }
    struct device_node *node = of_find_node_by_path("/soc/rtc@0x50000000");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    node = NULL;
    node = of_find_node_by_compatible("wq,rtc");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    // uintptr_t base, size;
    // if (of_get_memory(&base, &size) == 0) {
    //     printk("Memory base: %xu, size: %xu\n", base, size);
    // }

    node = of_find_node_by_compatible("wq,uart");
    node = of_get_interrupt_parent(node);
    if (node) {
        printk("interrupt-parent = %s\n", node->name);
    }
    // extern int of_scan_memory();
    // extern void memblock_dump();
    // of_scan_memory();
    // memblock_dump();
}