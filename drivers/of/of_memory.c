#include "os/types.h"
#include <mm/memblock.h>
#include <os/of.h>
#include <os/bswap.h>
#include <os/printk.h>

// 解析普通内存节点（只给 /memory 使用）
static int parse_memory_reg(struct device_node *node) {
    uint32_t address_cells, size_cells;
    struct device_prop *reg_prop;
    uint32_t *reg;
    uint32_t len, i;
    phys_addr_t start;
    size_t size;

    if (!node || !node->parent)
        return -1;

    // 关键：memory 节点从 根节点/父节点 拿 cells
    address_cells = of_get_address_cells(node->parent);
    size_cells = of_get_size_cells(node->parent);

    // printk("Parsing memory node: %s, address_cells=%d, size_cells=%d\n", node->full_path, address_cells, size_cells);

    reg_prop = of_get_property_by_name(node, "reg");
    if (!reg_prop) return -1;

    reg = of_get_reg(node);
    if (!reg) return -1;
 
    len = reg_prop->length / sizeof(uint32_t);

    for (i = 0; i + address_cells + size_cells <= len; i += address_cells + size_cells) {
        // 解析地址
        if (address_cells == 2)
            start = ((phys_addr_t)be32_to_cpu(reg[i]) << 32) | be32_to_cpu(reg[i+1]);
        else if (address_cells == 1)
            start = be32_to_cpu(reg[i]);
        else return -1;

        // 解析大小
        if (size_cells == 2)
            size = ((size_t)be32_to_cpu(reg[i+address_cells]) << 32) | be32_to_cpu(reg[i+address_cells+1]);
        else if (size_cells == 1)
            size = be32_to_cpu(reg[i+address_cells]);
        else return -1;

        // 普通内存：添加可用内存
        memblock_add(start, size);

        if (of_get_property_by_name(node, "no-map"))
            memblock_mark_nomap(start, size);
    }
    return 0;
}

// 解析保留内存（只给 /reserved-memory 使用）
static int parse_reserved_memory_reg(struct device_node *node) {
    uint32_t address_cells, size_cells;
    struct device_prop *reg_prop;
    uint32_t *reg;
    uint32_t len, i;
    phys_addr_t start;
    size_t size;

    if (!node) return -1;

    // 保留内存自己有 cells，直接读
    address_cells = of_get_address_cells(node);
    size_cells = of_get_size_cells(node);

    reg_prop = of_get_property_by_name(node, "reg");
    if (!reg_prop) return -1;

    reg = of_get_reg(node);
    if (!reg) return -1;
        printk("reg prop length: %xu bytes,\n", reg_prop->length);

    len = reg_prop->length / sizeof(uint32_t);

    for (i = 0; i + address_cells + size_cells <= len; i += address_cells + size_cells) {
        if (address_cells == 1)
            start = be32_to_cpu(reg[i]);
        else return -1;

        if (size_cells == 1)
            size = be32_to_cpu(reg[i+address_cells]);
        else return -1;


        memblock_reserve(start, size);

        if (of_get_property_by_name(node, "no-map"))
            memblock_mark_nomap(start, size);
    }
    return 0;
}

// 递归遍历：普通内存
static void memory_node_parse_recursive(struct device_node *node) {
    if (!node) return;
    parse_memory_reg(node); // 只添加可用内存

    struct device_node *child;
    for (child = node->children; child; child = child->sibling)
        memory_node_parse_recursive(child);
}

// 递归遍历：保留内存
static void reserved_memory_parse_recursive(struct device_node *node) {
    if (!node) return;
    parse_reserved_memory_reg(node); // 只保留，不添加

    struct device_node *child;
    for (child = node->children; child; child = child->sibling)
        reserved_memory_parse_recursive(child);
}

// ===================== 对外接口 =====================
int of_scan_memory() {
    struct device_node *node = of_find_node_by_path("/memory");
    if (!node) return -1;
    memory_node_parse_recursive(node);
    return 0;
}

int of_scan_reserved_memory() {
    struct device_node *node = of_find_node_by_path("/reserved-memory");
    if (!node) return -1;
    reserved_memory_parse_recursive(node);
    return 0;
}