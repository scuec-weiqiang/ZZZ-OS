/**
 * @FilePath     : /ZZZ-OS/arch/arm/include/asm/pgtbl.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-09 22:09:43
 * @LastEditTime : 2026-03-15 01:52:40
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef _MM_H
#define _MM_H

#include <os/types.h>
#include <os/mm/pgtbl_types.h>
#include <os/mm/pgprot.h>

void arch_pgtbl_init(pgtable_t *tbl); // 初始化页表

phys_addr_t arch_pgtbl_entry_get_pa(pgtable_t *tbl, uint32_t level,pgdesc_type_t type, pte_t *entry); // pte 值转物理地址
uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va);// 根据虚拟地址和级别计算页表索引
void arch_pgtbl_entry_set_flags(pgtable_t *tbl, int level, pte_t* entry, pgprot_t flags);
pgprot_t  arch_pgtbl_entry_get_flags(pgtable_t *tbl, int level, pte_t *entry);

void arch_pgtbl_set_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry, phys_addr_t pa, pgprot_t flags); // 设置 PTE 条目
void arch_pgtbl_clear_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry); // 清除 PTE 条目

bool arch_pgtbl_entry_is_valid(pte_t *entry); // 检查 PTE 是否有效
bool arch_pgtbl_entry_is_leaf(pte_t *entry); // 检查 PTE 是否为叶子节点

void arch_pgtbl_sync_range(void *addr, size_t size);
void arch_pgtbl_flush(void); // 刷新页表缓存（如 TLB）
void arch_pgtbl_switch_to(pgtable_t *pgtbl); // 切换到指定页表

#endif