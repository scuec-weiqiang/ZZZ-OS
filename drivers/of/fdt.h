/**
 * @FilePath: /ZZZ-OS/drivers/of/fdt.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-20 20:19:54
 * @LastEditTime: 2025-10-20 22:25:34
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
    u32 magic;             // 0xd00dfeed
    u32 totalsize;
    u32 off_dt_struct;     // structure block 偏移
    u32 off_dt_strings;    // strings block 偏移
    u32 off_mem_rsvmap;
    u32 version;
    u32 last_comp_version;
};

enum fdt_token {
    FDT_BEGIN_NODE = 1,
    FDT_END_NODE   = 2,
    FDT_PROP       = 3,
    FDT_NOP        = 4,
    FDT_END        = 9
};
/*
BEGIN_NODE "soc"
  PROP "compatible" = "simple-bus"
  BEGIN_NODE "uart@2020000"
    PROP "compatible" = "fsl,imx6ull-uart"
    PROP "reg" = <0x02020000 0x4000>
  END_NODE
END_NODE
END
*/

struct fdt_node {
    const char *name;
    const void *begin; // 指向FDT_BEGIN_NODE处
    const void *props; // 指向第一个属性处
    const void *end; // 指向FDT_END_NODE处
};


int fdt_init(void *dtb);
// struct fdt_node *fdt_find_node(const char *path);
const void *fdt_get_prop(struct fdt_node *node, const char *name, int *len);
u32 fdt_read_u32(const void *data);
const char *fdt_read_str(const void *data);
#endif