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

/* 通用页表标志 */
typedef enum pgtable_flags{
    PG_TABLE_FLAG_NONE     = 0,
    PG_TABLE_FLAG_USER     = 1 << 0,    // 用户可访问
    PG_TABLE_FLAG_WRITABLE = 1 << 1,    // 可写
    PG_TABLE_FLAG_READABLE = 1 << 2,    // 可读
    PG_TABLE_FLAG_EXEC     = 1 << 3,    // 可执行
    PG_TABLE_FLAG_DIRTY    = 1 << 4,    // 脏页
    PG_TABLE_FLAG_ACCESSED = 1 << 5,    // 已访问
    PG_TABLE_FLAG_COW      = 1 << 6,    // 写时复制
    PG_TABLE_FLAG_SHARED   = 1 << 7,    // 共享页面
    PG_TABLE_FLAG_DEVICE   = 1 << 8,    // 设备内存
    PG_TABLE_FLAG_NO_CACHE = 1 << 9,    // 无缓存
}pgtable_flags_t;

/* 页表层级描述 */
struct pgtable_level {
    int level;              // 层级号（从0开始）
    int bits;               // 该层级占用的虚拟地址位数
    size_t page_size;       // 该层级对应的页面大小
    const char *name;       // 层级名称
};

/* 架构页表特性 */
struct pgtable_features {
    int va_bits;            // 虚拟地址位数（如39、48）
    int pa_bits;            // 物理地址位数
    int levels;             // 页表层级数
    int page_shift;         // 基础页面移位（通常12）
    size_t page_size;       // 基础页面大小（通常4KB）
    
    // 支持的页面大小
    size_t supported_page_sizes[8];
    int num_supported_sizes;
    
    // 特性标志
    unsigned int features;
#define PGTABLE_FEATURE_HUGE_PAGES   (1 << 0)
#define PGTABLE_FEATURE_ASID         (1 << 1)
#define PGTABLE_FEATURE_GLOBAL_PAGES (1 << 2)
#define PGTABLE_FEATURE_NO_EXEC      (1 << 3)
#define PGTABLE_FEATURE_USER_PAGES   (1 << 4)
#define PGTABLE_FEATURE_ACCESSED_DIRTY (1 << 5)
    
    // 层级信息
    struct pgtable_level *level;
};

typedef struct pgtable{
    const char name[32]; // 页表名称
    uint32_t asid;          // 地址空间 id 
    void* root;           // 根页表地址（物理 or 虚拟由架构决定）
    const struct pgtable_features *features;
    struct pgtable_operations *ops;
}pgtable_t;

#endif // __KERNEL_PGTBL_H