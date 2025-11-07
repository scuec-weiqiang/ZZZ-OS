#include <drivers/of/fdt.h>
#include <os/printk.h>
#include <os/string.h>
#include <fs/path.h>
#include <os/malloc.h>
#include <os/bswap.h>

struct device_node *of_find_node_by_path(const char *path) {
    if (!root_node || !path)
        return NULL;
    struct device_node *current_node = (struct device_node *)root_node;
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
    if (!root_node || !compatible_prop)
        return NULL;

    // 使用队列进行深度优先搜索
    struct device_node **queue = (struct device_node **)malloc(sizeof(struct device_node *) * 512);
    int front = 0, rear = 0;

    queue[rear] = (struct device_node *)root_node;
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

struct device_node *of_find_node_by_phandle(uint32_t phandle) {
    if (!root_node)
        return NULL;

    if (phandle <= PHANDLE_MAX - 1) {
        return phandle_table[phandle];
    }

    int front = 0;
    int rear = 0;
    struct device_node **queue = (struct device_node **)malloc(sizeof(struct device_node *) * 512);
    queue[rear] = (struct device_node *)root_node;
    rear++;

    while (front < rear) {
        struct device_node *curr = queue[front];
        front++;

        uint32_t curr_phandle = fdt_get_phandle(curr);
        if (phandle == curr_phandle) {
            free(queue);
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

int of_get_memory(uintptr_t *base, uintptr_t *size) {
    struct device_node *memory_node = of_find_node_by_path("/memory");
    if (!memory_node) {
        return -1;
    }

    uint32_t address_cells = of_get_address_cells(memory_node);
    uint32_t size_cells = of_get_size_cells(memory_node);
    uint32_t *reg_values = of_get_reg(memory_node);

    uintptr_t bs = 0;
    uintptr_t sz = 0;

    for (uint32_t i = 0; i < address_cells; i++) {
        bs = (bs << 32) | be32_to_cpu(reg_values[i]);
    }
    for (uint32_t i = 0; i < size_cells; i++) {
        sz = (sz << 32) | be32_to_cpu(reg_values[address_cells + i]);
    }

    *base = bs;
    *size = sz;

    return 0;
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


void of_test() {
   
    fdt_walk_node(root_node, 0);
    struct device_node *node = of_find_node_by_path("/soc/rtc@0x50000000");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    node = NULL;
    node = of_find_node_by_compatible("wq,rtc");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    uintptr_t base, size;
    if (of_get_memory(&base, &size) == 0) {
        printk("Memory base: %xu, size: %xu\n", base, size);
    }

    node = of_find_node_by_compatible("wq,uart");
    node = of_get_interrupt_parent(node);
    if (node) {
        printk("interrupt-parent = %s\n", node->name);
    }
}