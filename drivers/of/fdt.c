/**
 * @FilePath: /ZZZ-OS/drivers/of/fdt.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:25:59
 * @LastEditTime: 2025-10-28 16:34:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright        : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "fdt.h"
#include "bswap.h"
#include "malloc.h"
#include "path.h"
#include "printk.h"
#include "string.h"

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define GET_NEXT_TOKEN(x, len) \
    __PROTECT((x) = (uint32_t *)((uintptr_t)x + ALIGN_UP(len, 4));)

static struct fdt_header *fdt;
static const char *struct_block;
static const char *strings;

#define PHANDLE_MAX 1
static struct fdt_node *phandle_table[PHANDLE_MAX] = {NULL};

static const struct fdt_node *root_node;

static struct fdt_node *new_fdt_node(const char *name, struct fdt_node *parent) {
    struct fdt_node *node = (struct fdt_node *)malloc(sizeof(struct fdt_node));
    node->name = strdup(name);
    node->full_path = NULL;
    node->parent = parent;
    node->children = NULL;
    node->sibling = NULL;
    node->properties = NULL;

    if (parent) {
        int name_len = strlen(name);
        int parent_path_len = parent ? strlen(parent->full_path) : 0;
        int path_len = parent_path_len + 1 + name_len + 1;

        node->full_path = (char *)malloc(path_len);
        memcpy(node->full_path, parent->full_path, parent_path_len);
        memcpy(node->full_path + parent_path_len, "/", 1);
        memcpy(node->full_path + parent_path_len + 1, name, name_len + 1);
    } else {
        node->full_path = strdup(name);
    }
    return node;
}

static void free_fdt_node(struct fdt_node *node) {
    if (!node)
        return;
    if (node->children != NULL) {
        struct fdt_node *child = node->children;
        while (child) {
            struct fdt_node *next = child->sibling;
            free_fdt_node(child);
            child = next;
        }
    }
    free(node->name);
    struct fdt_prop *prop = node->properties;
    while (prop) {
        struct fdt_prop *next = prop->next;
        free(prop->name);
        free(prop);
        prop = next;
    }
    free(node);
}

static struct fdt_prop *new_fdt_prop(const char *name, uint32_t len, const void *value) {
    if (!name || !value)
        return NULL;
    struct fdt_prop *prop = (struct fdt_prop *)malloc(sizeof(struct fdt_prop));
    prop->name = strdup(name);
    prop->length = len;
    prop->value = malloc(len);
    memcpy(prop->value, value, len);
    prop->next = NULL;
    return prop;
}

static void free_fdt_prop(struct fdt_prop *prop) {
    if (!prop)
        return;
    free(prop->name);
    free(prop->value);
    free(prop);
}

static int add_fdt_prop(struct fdt_node *node, struct fdt_prop *prop) {
    if (!node || !prop)
        return -1;
    if (!node->properties) {
        node->properties = prop;
    } else {
        struct fdt_prop *p = node->properties;
        while (p->next)
            p = p->next;
        p->next = prop;
    }
    return 0;
}

static int add_fdt_child(struct fdt_node *parent, struct fdt_node *child) {
    if (!parent || !child)
        return -1;
    if (!parent->children) {
        parent->children = child;
    } else {
        struct fdt_node *n = parent->children;
        while (n->sibling)
            n = n->sibling;
        n->sibling = child;
    }
    return 0;
}

static struct fdt_node *parse_struct_block(const char *struct_block, char *strings) {
    struct fdt_node *root = NULL;
    struct fdt_node *curr = NULL;
    uint32_t *p = (uint32_t *)struct_block;

    while (1) {
        uint32_t token = be32_to_cpu(*p);
        p++;
        switch (token) {
        case FDT_BEGIN_NODE: {
            const char *name = (const char *)p;
            p++;
            printk("in node: %s\n", name);
            struct fdt_node *new_node = (struct fdt_node *)new_fdt_node(name, curr);
            if (root == NULL) {
                root = new_node;
            } else {
                add_fdt_child(curr, new_node);
            }
            curr = new_node;
            break;
        }
        case FDT_PROP: {
            uint32_t len = be32_to_cpu(*p);
            p++;
            uint32_t nameoff = be32_to_cpu(*p);
            p++;
            const char *prop_name = strings + nameoff;
            struct fdt_prop *new_prop = new_fdt_prop(prop_name, len, p);
            if (strcmp(new_prop->name, "phandle") == 0) {
                uint32_t phandle = be32_to_cpu(*(uint32_t *)new_prop->value);
                if (phandle < PHANDLE_MAX - 1) {
                    phandle_table[phandle] = curr; // 只存一定数值的phandle，超出的部分等到要用的时候再解析
                }
            }
            add_fdt_prop(curr, new_prop);
            p += (len + 3) / 4; // 跳过属性值
            break;
        }
        case FDT_NOP:
            break;
        case FDT_END_NODE:
            if (curr)
                curr = curr->parent;
            break;
        case FDT_END:
            return root;
        default:
            break; // 其他
        }
    }
    free_fdt_node(root);
    return NULL;
}

static struct fdt_node *find_child_node_by_name(const struct fdt_node *parent, const char *name) {
    if (!parent || !name)
        return NULL;

    struct fdt_node *child = parent->children;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
            break;
        }
        child = child->sibling;
    }
    return NULL;
}

struct fdt_node *fdt_find_node_by_path(const char *path) {
    if (!root_node || !path)
        return NULL;
    struct fdt_node *current_node = (struct fdt_node *)root_node;
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

struct fdt_node *fdt_find_node_by_compatible(const char *compatible_prop) {
    if (!root_node || !compatible_prop)
        return NULL;

    // 使用队列进行深度优先搜索
    struct fdt_node **queue = (struct fdt_node **)malloc(sizeof(struct fdt_node *) * 512);
    int front = 0, rear = 0;

    queue[rear] = (struct fdt_node *)root_node;
    rear++;

    while (front < rear) {
        struct fdt_node *current_node = queue[front];
        front++;

        // 检查当前节点的 compatible 属性
        struct fdt_prop *prop = current_node->properties;
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
        struct fdt_node *child = current_node->children;
        while (child) {
            queue[rear] = child;
            rear++;
            child = child->sibling;
        }
    }

    free(queue);
    return NULL;
}

struct fdt_prop *fdt_get_prop_by_name(const struct fdt_node *node, const char *name) {
    if (!node || !name)
        return NULL;

    struct fdt_prop *prop = node->properties;
    while (prop) {
        if (strcmp(prop->name, name) == 0) {
            return prop;
        }
        prop = prop->next;
    }
    return NULL;
}

uint32_t fdt_get_phandle(const struct fdt_node *node) {
    if (!node)
        return 0;
    struct fdt_prop *prop = fdt_get_prop_by_name(node, "phandle");
    if (prop) {
        return be32_to_cpu(*(uint32_t *)prop->value);
    }
    return 0;
}

uint32_t *fdt_get_reg(const struct fdt_node *node) {
    if (!node)
        return 0;
    struct fdt_prop *prop = fdt_get_prop_by_name(node, "reg");
    if (prop) {
        return (uint32_t *)prop->value;
    }
    return 0;
}

struct fdt_node *fdt_find_node_by_phandle(uint32_t phandle) {
    if (!root_node)
        return NULL;

    if (phandle <= PHANDLE_MAX - 1) {
        return phandle_table[phandle];
    }

    int front = 0;
    int rear = 0;
    struct fdt_node **queue = (struct fdt_node **)malloc(sizeof(struct fdt_node *) * 512);
    queue[rear] = (struct fdt_node *)root_node;
    rear++;

    while (front < rear) {
        struct fdt_node *curr = queue[front];
        front++;

        uint32_t curr_phandle = fdt_get_phandle(curr);
        if (phandle == curr_phandle) {
            free(queue);
            return curr;
        }

        struct fdt_node *child = curr->children;
        while (child) {
            queue[rear] = child;
            rear++;
            child = child->sibling;
        }
    }
    free(queue);
    return NULL;
}

uint32_t fdt_get_address_cells(const struct fdt_node *node) {
    if (!node)
        return -1;
    struct fdt_node *current = (struct fdt_node *)node->parent;
    struct fdt_prop *prop = NULL;

    while (current) {
        prop = fdt_get_prop_by_name(current, "#address-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(uint32_t *)prop->value);
        }
    }
    return 2;
}

uint32_t fdt_get_size_cells(const struct fdt_node *node) {
    if (!node)
        return -1;
    struct fdt_node *current = (struct fdt_node *)node->parent;
    struct fdt_prop *prop = NULL;

    while (current) {
        prop = fdt_get_prop_by_name(current, "#size-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(uint32_t *)prop->value);
        }
    }
    return 2;
}

int fdt_get_memory(uintptr_t *base, uintptr_t *size) {
    struct fdt_node *memory_node = fdt_find_node_by_path("/memory");
    if (!memory_node) {
        return -1;
    }

    uint32_t address_cells = fdt_get_address_cells(memory_node);
    uint32_t size_cells = fdt_get_size_cells(memory_node);
    struct fdt_prop *reg_prop = fdt_get_prop_by_name(memory_node, "reg");
    if (!reg_prop || reg_prop->length < 16) {
        return -1;
    }

    printk("#address-cells = %d\n", address_cells);
    printk("#size-cells = %d\n", size_cells);
    uint32_t *reg_values = (uint32_t *)reg_prop->value;

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

struct fdt_node *fdt_get_interrupt_parent(const struct fdt_node *node) {
    if (!node)
        return NULL;
    struct fdt_prop *prop = fdt_get_prop_by_name(node, "interrupt-parent");
    if (!prop) {
        return NULL;
    }

    uint32_t phandle = be32_to_cpu(*(uint32_t *)prop->value);
    return fdt_find_node_by_phandle(phandle);
}

int fdt_walk_node(const struct fdt_node *node, int level) {
    if (!node)
        return 0;

    for (int i = 0; i < level; i++) {
        printk("  ");
    }
    level++;
    printk("%s\n", node->name);
    struct fdt_node *curr = node->children;
    while (curr) {
        fdt_walk_node(curr, level + 1);
        curr = curr->sibling;
    }
    level--;
    return level;
}

int fdt_init(void *dtb) {
    fdt = (struct fdt_header *)dtb;
    if (be32_to_cpu(fdt->magic) != FDT_MAGIC) {
        printk("Bad FDT magic!\n");
        return -1;
    }

    struct_block = (char *)dtb + (size_t)be32_to_cpu(fdt->off_dt_struct);
    strings = (char *)dtb + (size_t)be32_to_cpu(fdt->off_dt_strings);
    return 0;
}

void fdt_test() {
    root_node = parse_struct_block(struct_block, (char *)strings);
    fdt_walk_node(root_node, 0);
    struct fdt_node *node = fdt_find_node_by_path("/soc/rtc@0x50000000");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    node = NULL;
    node = fdt_find_node_by_compatible("wq,rtc");
    if (node) {
        printk("Found node: %s\n", node->name);
    }
    uintptr_t base, size;
    if (fdt_get_memory(&base, &size) == 0) {
        printk("Memory base: %xu, size: %xu\n", base, size);
    }

    node = fdt_find_node_by_compatible("wq,uart");
    node = fdt_get_interrupt_parent(node);
    if (node) {
        printk("interrupt-parent = %s\n", node->name);
    }
}