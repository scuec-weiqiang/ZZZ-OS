/**
 * @FilePath: /ZZZ-OS/drivers/of/fdt.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:25:59
 * @LastEditTime: 2025-11-17 20:10:01
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright        : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <drivers/of/fdt.h>
#include <os/bswap.h>
#include <os/kmalloc.h>
#include <fs/path.h>
#include <os/printk.h>
#include <os/string.h>
#include <stdint.h>

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define GET_NEXT_TOKEN(x, len) \
    __PROTECT((x) = (uint32_t *)((uintptr_t)x + ALIGN_UP(len, 4));)

static struct fdt_header *fdt;
static const char *struct_block;
static const char *strings;

struct device_node *phandle_table[PHANDLE_MAX] = {NULL};
const struct device_node *fdt_root_node;

struct device_node *fdt_new_node(const char *name, struct device_node *parent) {
    struct device_node *node = (struct device_node *)kmalloc(sizeof(struct device_node));
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

        node->full_path = (char *)kmalloc(path_len);
        memcpy(node->full_path, parent->full_path, parent_path_len);
        memcpy(node->full_path + parent_path_len, "/", 1);
        memcpy(node->full_path + parent_path_len + 1, name, name_len + 1);
    } else {
        node->full_path = (char*)name;
    }
    return node;
}

void fdt_free_node(struct device_node *node) {
    if (!node)
        return;
    if (node->children != NULL) {
        struct device_node *child = node->children;
        while (child) {
            struct device_node *next = child->sibling;
            fdt_free_node(child);
            child = next;
        }
    }
    kfree(node->name);
    struct device_prop *prop = node->properties;
    while (prop) {
        struct device_prop *next = prop->next;
        kfree(prop->name);
        kfree(prop);
        prop = next;
    }
    kfree(node);
}

struct device_prop *fdt_new_prop(const char *name, uint32_t len, const void *value) {
    if (!name || !value)
        return NULL;
    struct device_prop *prop = (struct device_prop *)kmalloc(sizeof(struct device_prop));
    prop->name = strdup(name);
    prop->length = len;
    prop->value = kmalloc(len);
    memcpy(prop->value, value, len);
    prop->next = NULL;
    return prop;
}

void fdt_free_prop(struct device_prop *prop) {
    if (!prop)
        return;
    kfree(prop->name);
    kfree(prop->value);
    kfree(prop);
}

int fdt_add_prop(struct device_node *node, struct device_prop *prop) {
    if (!node || !prop)
        return -1;
    if (!node->properties) {
        node->properties = prop;
    } else {
        struct device_prop *p = node->properties;
        while (p->next)
            p = p->next;
        p->next = prop;
    }
    return 0;
}

int fdt_add_child(struct device_node *parent, struct device_node *child) {
    if (!parent || !child)
        return -1;
    if (!parent->children) {
        parent->children = child;
    } else {
        struct device_node *n = parent->children;
        while (n->sibling)
            n = n->sibling;
        n->sibling = child;
    }
    return 0;
}

struct device_node *parse_struct_block(const char *struct_block, char *strings) {
    struct device_node *root = NULL;
    struct device_node *curr = NULL;
    uint32_t *p = (uint32_t *)struct_block;

    while (1) {
        uint32_t token = be32_to_cpu(*p);
        p++;
        switch (token) {
        case FDT_BEGIN_NODE: {
            const char *name = (const char *)p;
            p++;
            // printk("in node: %s\n", name);
            struct device_node *new_node = (struct device_node *)fdt_new_node(name, curr);
            if (root == NULL) {
                root = new_node;
            } else {
                fdt_add_child(curr, new_node);
            }
            curr = new_node;
            curr->depth = curr->parent ? curr->parent->depth + 1 : 0;
            break;
        }
        case FDT_PROP: {
            uint32_t len = be32_to_cpu(*p);
            p++;
            uint32_t nameoff = be32_to_cpu(*p);
            p++;
            const char *prop_name = strings + nameoff;
            struct device_prop *new_prop = fdt_new_prop(prop_name, len, p);
            if (strcmp(new_prop->name, "phandle") == 0) {
                uint32_t phandle = be32_to_cpu(*(uint32_t *)new_prop->value);
                if (phandle < PHANDLE_MAX - 1) {
                    phandle_table[phandle] = curr; // 只存一定数值的phandle，超出的部分等到要用的时候再解析
                }
            }
            fdt_add_prop(curr, new_prop);
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
    fdt_free_node(root);
    return NULL;
}

struct device_node *find_child_node_by_name(const struct device_node *parent, const char *name) {
    if (!parent || !name)
        return NULL;

    struct device_node *child = parent->children;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
            break;
        }
        child = child->sibling;
    }
    return NULL;
}

int fdt_walk_node(const struct device_node *node, int level) {
    if (!node)
        return 0;

    for (int i = 0; i < level; i++) {
        printk("  ");
    }
    level++;
    printk("%s\n", node->name);
    struct device_node *curr = node->children;
    while (curr) {
        fdt_walk_node(curr, level + 1);
        curr = curr->sibling;
    }
    level--;
    return level;
}

static struct device_node *queue[sizeof(struct device_node *) * 512] = {0};
int fdt_walk(struct device_node *node, struct list_head *list) {
    if (!node && !list)
        return -1;

    int front = 0, rear = 0;

    queue[rear] = node;
    rear++;

    while (front < rear) {
        struct device_node *current_node = queue[front];
        front++;

        list_add_tail(list,&current_node->node);

        // 将子节点加入队列
        struct device_node *child = current_node->children;
        while (child) {
            queue[rear] = child;
            rear++;
            child = child->sibling;
        }
    }
    return 0;
}


int fdt_init(void *dtb) {
    fdt = (struct fdt_header *)dtb;
    if (be32_to_cpu(fdt->magic) != FDT_MAGIC) {
        printk("Bad FDT magic!\n");
        return -1;
    }

    struct_block = (char *)dtb + (size_t)be32_to_cpu(fdt->off_dt_struct);
    strings = (char *)dtb + (size_t)be32_to_cpu(fdt->off_dt_strings);
    fdt_root_node = parse_struct_block(struct_block, (char *)strings);

    return 0;
}

