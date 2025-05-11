/**
 * @FilePath: /ZZZ/arch/riscv64/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-09 00:45:04
 * @LastEditTime: 2025-05-10 01:45:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _MM_H
#define _MM_H

#include "types.h"

#define PTE_V (1 << 0)      // 有效位
#define PTE_R (1 << 1)      // 可读
#define PTE_W (1 << 2)      // 可写
#define PTE_X (1 << 3)      // 可执行
#define PTE_U (1 << 4)      // 用户模式可访问

#define PAGE_SIZE 4096    // 页大小
#define PAGE_SHIFT 12     // 页偏移量
#define PAGE_MASK (~(PAGE_SIZE - 1))   // 页掩码

#define PTE_FLAGS (PTE_V | PTE_R | PTE_W | PTE_X)   // 页表项标志位

#define PA2PTE(pa) (((uint64_t)(pa) >> 12) << 10)
#define PTE2PA(pte) (((pte&0x3fffffffffffff) >> 10) << 12)

#define SATP_SV39 (8L << 60)
#define SATP_MODE SATP_SV39 
#define MAKE_SATP(pagetable) (SATP_MODE | (((uint64_t)pagetable) >> 12))

typedef uint64_t pte_t;
typedef pte_t pgtbl_t;

#endif