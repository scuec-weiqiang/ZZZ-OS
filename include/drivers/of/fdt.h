/**
 * @FilePath: /ZZZ-OS/drivers/of/fdt.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:19:54
 * @LastEditTime: 2025-10-28 15:38:01
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

/*
+---------------------------+
| struct fdt_header         |  固定头部
+---------------------------+
| memory reservation block  |
+---------------------------+
| structure block           |  节点树（BEGIN_NODE、PROP、END_NODE）
+---------------------------+
| strings block             |  属性名字符串表
+---------------------------+
*/
#ifndef FDT_H
#define FDT_H

#include "types.h"

#define FDT_MAGIC 0xd00dfeed

struct fdt_header {
    uint32_t magic;             // 0xd00dfeed
    uint32_t totalsize;
    uint32_t off_dt_struct;     // structure block 偏移
    uint32_t off_dt_strings;    // strings block 偏移
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
};

enum fdt_token {
    FDT_BEGIN_NODE = 1,
    FDT_END_NODE   = 2,
    FDT_PROP       = 3,
    FDT_NOP        = 4,
    FDT_END        = 9
};

// 属性结构
struct fdt_prop {
    char *name;
    uint32_t length;
    void *value;
    struct fdt_prop *next;
};

struct fdt_node {
    char *name;
    char *full_path; // 全路径（如 /soc/uart@12340000）
    struct fdt_node *parent;
    struct fdt_node *children;
    struct fdt_node *sibling;
    struct fdt_prop *properties;
};

extern int fdt_init(void *dtb);
extern void fdt_test();

extern struct fdt_node* fdt_find_node_by_path(const char* path);
extern struct fdt_node* fdt_find_node_by_compatible(const char* compatible_prop);
extern struct fdt_prop* fdt_get_prop_by_name(const struct fdt_node* node, const char* name);
extern struct fdt_node* fdt_find_node_by_phandle(uint32_t phandle);
extern uint32_t fdt_get_address_cells(const struct fdt_node *node);
extern uint32_t fdt_get_size_cells(const struct fdt_node *node);
extern uint32_t* fdt_get_reg(const struct fdt_node *node);
extern int fdt_get_memory(uintptr_t *base, uintptr_t *size);
extern struct fdt_node* fdt_get_interrupt_parent(const struct fdt_node *node);
#endif