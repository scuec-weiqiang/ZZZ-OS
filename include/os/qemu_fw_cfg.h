/**
 * @FilePath     : /ZZZ-OS/include/os/qemu_fw_cfg.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-17 16:20:02
 * @LastEditTime : 2026-07-18 01:05:48
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __OS_QEMU_FW_CFG_H
#define __OS_QEMU_FW_CFG_H
#include <os/types.h>

// QEMU FW_CFG MMIO registers
// base + 0x00   data register
// base + 0x08   selector register
// base + 0x10   DMA address register

#define FW_CFG_DMA_CTL_ERROR  0x01
#define FW_CFG_DMA_CTL_READ   0x02
#define FW_CFG_DMA_CTL_SKIP   0x04
#define FW_CFG_DMA_CTL_SELECT 0x08
#define FW_CFG_DMA_CTL_WRITE  0x10
#define FW_CFG_FILE_DIR 0x0019

struct fw_cfg_file {
    __be32 size;
    __be16 select;
    __be16 reserved;
    char name[56];
} __packed;

struct fw_cfg_files {
    __be32 count;
    struct fw_cfg_file files[];
} __packed;

int qemu_fw_cfg_init(phys_addr_t base);
int qemu_fw_cfg_find_file(const char *name,
                          u16 *selector,
                          u32 *size);
int qemu_fw_cfg_write_file(const char *name,
                           const void *data,
                           size_t size);
int qemu_fw_cfg_dma_write(u16 selector,
                          const void *data,
                          size_t size);
bool qemu_fw_cfg_ready(void);


struct fw_cfg_dma_access {
    __be32 control;
    __be32 length;
    __be64 address;
} __packed;

struct ramfb_config {
    __be64 addr;
    __be32 fourcc;
    __be32 flags;
    __be32 width;
    __be32 height;
    __be32 stride;
} __packed;

struct fb_info;
int qemu_ramfb_configure(struct fb_info *info);
#endif
