/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/drivers/of/fdt.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:25:59
 * @LastEditTime: 2025-10-23 14:25:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright        : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "fdt.h"
#include "bswap.h"
#include "malloc.h"
#include "printk.h"
#include "string.h"

#define be32_to_cpu(x) __bswapsi2(x)
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define GET_NEXT_TOKEN(x, len)                                                                                                 \
    __PROTECT((x) = (u32 *)((uintptr_t)x + ALIGN_UP(len, 4));)

static struct fdt_header *fdt;
static const char *struct_block;
static const char *strings;

static const struct fdt_node *root_node;

static struct fdt_node *new_fdt_node(const char *name, struct fdt_node *parent) {
    struct fdt_node *node = (struct fdt_node *)malloc(sizeof(struct fdt_node));
    node->name = strdup(name);
    node->full_path = NULL;
    node->parent = parent;
    node->children = NULL;
    node->sibling = NULL;
    node->properties = NULL;
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

static struct fdt_prop *new_fdt_prop(const char *name, u32 len, const void *value) {
    struct fdt_prop *prop = (struct fdt_prop *)malloc(sizeof(struct fdt_prop));
    prop->name = strdup(name);
    prop->length = len;
    prop->value = (void *)value;
    prop->next = NULL;
    return prop;
}

static void free_fdt_prop(struct fdt_prop *prop) {
    if (!prop)
        return;
    free(prop->name);
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

static struct fdt_node *parse_struct_block(const char *struct_block,char *strings) {
    struct fdt_node *root = NULL;
    struct fdt_node *curr = NULL;
    u32 *p = (u32 *)struct_block + 1; // 跳过FDT_BEGIN_NODE

    while (1) {
        u32 token = be32_to_cpu(*p);
        p++;
        switch (token) {
            case FDT_BEGIN_NODE: {
                const char *name = (const char *)p;p++;
                printk("in node: %s\n", name);
                struct fdt_node *new_node = (struct fdt_node *)new_fdt_node(name, curr);
                if (root == NULL)
                    root = new_node;
                else 
                    add_fdt_child(curr, new_node);
                curr = new_node;
                break;
            }
            case FDT_PROP: {
                u32 len = be32_to_cpu(*p);p++;
                u32 nameoff = be32_to_cpu(*p);p++;
                const char *prop_name = strings + nameoff;
                struct fdt_prop *new_prop = new_fdt_prop(prop_name, len, p);
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

static void *fdt_find_node(void *parent, const char *child_name) {
    u32 *p = (u32 *)parent + 1; // 跳过FDT_BEGIN_NODE
    int depth = 0;

    while (1) {
        u32 token = be32_to_cpu(*p);
        p++;
        switch (token) {
        case FDT_BEGIN_NODE: {
            const char *name = (const char *)p;
            printk("in node: %s\n", name);
            depth++;
            if (strcmp(name, child_name) == 0 && depth == 1) {
                printk("find node: %s\n", name);
                return (void *)(p - 1);
            }
            break;
        }
        case FDT_PROP: {
            u32 len = be32_to_cpu(*p);
            p++;
            u32 nameoff = be32_to_cpu(*p);
            p++;
            p += (len + 3) / 4; // 跳过属性值
            break;
        }
        case FDT_NOP:
            break;
        case FDT_END_NODE:
            depth--;
            if (depth == 0) {
                return NULL;
            }
            break;
        case FDT_END:
            return NULL;
        default:
            break; // 其他
        }
    }
    return NULL;
}

static void *fdt_get_property(void *node, const char *prop_name, u32 *out_len) {
    u32 *p = (u32 *)node + 1; // 跳过FDT_BEGIN_NODE
    int depth = 0;

    while (1) {
        u32 token = be32_to_cpu(*p);
        p++;
        switch (token) {
        case FDT_BEGIN_NODE: {
            depth++;
            break;
        }
        case FDT_PROP: {
            u32 len = be32_to_cpu(*p);
            p++;
            u32 nameoff = be32_to_cpu(*p);
            p++;
            if (strcmp((const char *)(strings + nameoff), prop_name) == 0 &&
                    depth == 0) {
                if (out_len)
                    *out_len = len;
                return (void *)p;
            }
            p += (len + 3) / 4; // 跳过属性值
            break;
        }
        case FDT_NOP:
            break;
        case FDT_END_NODE:
            depth--;
            break;
        case FDT_END:
            return NULL;
        default:
            break; // 其他
        }
    }
    return NULL;
}

int walk_node(const struct fdt_node *node,int level){
    if (!node) return 0;
    for (int i = 0; i < level; i++) {
        printk("  ");
    }
    printk("%s\n", node->name);
    struct fdt_node *curr = node->children;
    while (curr) {
        walk_node(curr, level + 1);
        curr = curr->sibling;
    }
    level --;
    return level;
}

void fdt_test() {
    root_node = parse_struct_block(struct_block, (char *)strings);
    walk_node(root_node, 0);
}