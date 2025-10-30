/**
 * @FilePath: /ZZZ-OS/arch/riscv64/asm/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 13:05:59
 * @LastEditTime: 2025-10-06 18:52:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _MM_H
#define _MM_H

#include <os/types.h>

#define PTE_V (1 << 0)      // 有效位
#define PTE_R (1 << 1)      // 可读
#define PTE_W (1 << 2)      // 可写
#define PTE_X (1 << 3)      // 可执行
#define PTE_U (1 << 4)      // 用户模式可访问

#define PAGE_SIZE 0x1000    // 页大小4096字节
#define PAGE_SHIFT 12     // 页偏移量
#define PAGE_MASK (~(PAGE_SIZE - 1))   // 页掩码

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#define PA2PTE(pa) (((uint64_t)(pa) >> 12) << 10)
#define PTE2PA(pte) (((pte&0xffffffffffffffff) >> 10) << 12)

#define SATP_SV39 (8L << 60)
#define SATP_MODE SATP_SV39 

#define KERNEL_PA_BASE 0x80000000
#define KERNEL_VA_BASE 0xffffffffc0000000
#define KERNEL_VA_START 0xffffffffc0200000
#define KERNEL_VA(pa) (KERNEL_VA_BASE + ((uint64_t)(pa)) - KERNEL_PA_BASE)
#define KERNEL_PA(va) ((uint64_t)(va) - KERNEL_VA_BASE + KERNEL_PA_BASE)

#define KERNEL_MMIO_BASE 0xFFFFFFBFC00000
#define KERNEL_MMIO_VA(pa) (KERNEL_MMIO_BASE + ((uint64_t)(pa)))

#endif