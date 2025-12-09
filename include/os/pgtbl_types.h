/**
 * @FilePath: /ZZZ-OS/include/os/pgtbl_types.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-12-04 19:04:15
 * @LastEditTime: 2025-12-05 16:21:39
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PGTBL_H
#define __KERNEL_PGTBL_H

#include <os/types.h>

/*
|    arch     | level |      struct           |
| ----------- | ----- | --------------------- |
| x86_64      |  4~5  | PGD → PUD → PMD → PTE |
| RISC-V Sv39 |  3    | PGD → PMD → PTE       |
| RISC-V Sv48 |  4    | PGD → PUD → PMD → PTE |
| ARM64       |  4    | L0 → L1 → L2 → L3     |
*/

typedef uintptr_t pteval_t;

// 每一层的 entry 本质相同，只在语义上不同
typedef struct { pteval_t val; } pte_t;
typedef struct { pteval_t val; } pmd_t;
typedef struct { pteval_t val; } pud_t;
typedef struct { pteval_t val; } pgd_t;

enum {
    PGTBL_KERNEL = 1 << 0,
    PGTBL_USER   = 1 << 1,
};

enum {
    PGTBL_R  = 1 << 0,
    PGTBL_W  = 1 << 1,
    PGTBL_X  = 1 << 2,
};

enum huge_page {
    HUGE_PAGE_4K = 1UL << 12,
    HUGE_PAGE_2M = 1UL << 21,
    HUGE_PAGE_1G = 1UL << 30,
};

typedef struct pgtbl{
    void* root;           // 根页表地址（物理 or 虚拟由架构决定）
    uint16_t levels;       // 页表级数（3,4,5...)
    uint16_t page_shift;   // 12 for 4K
    uint32_t asid;          // 地址空间 id
    uint32_t flags;         // arch private
} pgtbl_t;


#endif // __KERNEL_PGTBL_H