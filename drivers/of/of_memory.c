/**
 * @FilePath: /ZZZ-OS/drivers/of/of_memory.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-14 16:36:20
 * @LastEditTime: 2025-11-17 00:13:07
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/mm/memblock.h>
#include <os/of.h>
#include <os/bswap.h>

int of_scan_memory() {
    struct device_node *memory_node = of_find_node_by_path("/memory");
    if (!memory_node) {
        return -1;
    }

    struct list_head list = LIST_HEAD_INIT(list);
    fdt_walk(memory_node, &list);

    struct device_node *pos;
    list_for_each_entry(pos, &list, struct device_node, node) {
        printk("Memory node: %s\n", pos->name);
        uint32_t address_cells = of_get_address_cells(pos);
        uint32_t size_cells = of_get_size_cells(pos);

        struct device_prop *reg_prop = of_get_prop_by_name(pos, "reg");
        if (!reg_prop) {
            continue;
        }
        uint32_t len = reg_prop->length / sizeof(uint32_t);

        uint32_t *reg = of_get_reg(pos);
        if (!reg) {
            return -1;
        }

        phys_addr_t start = 0;
        size_t size = 0;

        for (int i = 0; i < len; i += address_cells + size_cells) {
            if (address_cells == 2) {
                start = ((uint64_t)be32_to_cpu(reg[i])) | be32_to_cpu(reg[i + 1] << 32);
            } else if (address_cells == 1) {
                start = be32_to_cpu(reg[i]);
            } else {
                return -1;
            }

            if (size_cells == 2) {
                size = ((uint64_t)be32_to_cpu(reg[i + address_cells])) | be32_to_cpu(reg[i + address_cells + 1] << 32);
            } else if (size_cells == 1) {
                size = be32_to_cpu(reg[i + address_cells]);
            } else {
                return -1;
            }
            
            memblock_add(start, size);
            if (of_get_prop_by_name(pos, "no-map")) {
                memblock_mark_nomap(start, size);
            }
        }
    }
    // memblock_dump();
    return 0;
}


int of_scan_reserved_memory() {
    struct device_node *memory_node = of_find_node_by_path("/reserved-memory");
    if (!memory_node) {
        return -1;
    }

    struct list_head list = LIST_HEAD_INIT(list);
    fdt_walk(memory_node, &list);

    struct device_node *pos;
    list_for_each_entry(pos, &list, struct device_node, node) {
        printk("Reserved memory node: %s\n", pos->name);

        uint32_t address_cells = of_get_address_cells(pos);
        uint32_t size_cells = of_get_size_cells(pos);

        struct device_prop *reg_prop = of_get_prop_by_name(pos, "reg");
        if (!reg_prop) {
            continue;
        }
        uint32_t len = reg_prop->length / sizeof(uint32_t);

        uint32_t *reg = of_get_reg(pos);
        if (!reg) {
            return -1;
        }

        phys_addr_t start = 0;
        size_t size = 0;

        for (int i = 0; i < len; i += address_cells + size_cells) {
            if (address_cells == 2) {
                start = ((uint64_t)be32_to_cpu(reg[i])) | be32_to_cpu(reg[i + 1] << 32);
            } else if (address_cells == 1) {
                start = be32_to_cpu(reg[i]);
            } else {
                return -1;
            }

            if (size_cells == 2) {
                size = ((uint64_t)be32_to_cpu(reg[i + address_cells])) | be32_to_cpu(reg[i + address_cells + 1] << 32);
            } else if (size_cells == 1) {
                size = be32_to_cpu(reg[i + address_cells]);
            } else {
                return -1;
            }
            
            memblock_reserve(start, size);
            if (of_get_prop_by_name(pos, "no-map")) {
                memblock_mark_nomap(start, size);
            }
        }
    }
    // memblock_dump();
    return 0;
}
