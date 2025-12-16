/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/pgtbl.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 13:05:59
 * @LastEditTime: 2025-10-31 00:05:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _MM_H
#define _MM_H

#include <os/types.h>
#include <os/mm/pgtbl_types.h>

int arch_pgtbl_init(pgtable_t *tbl); // 初始化页表
pteval_t arch_pgtbl_pa_to_pteval(phys_addr_t pa); // 物理地址转 pte 值
phys_addr_t arch_pgtbl_pteval_to_pa(pteval_t val); // pte 值转物理地址
uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va); // 计算某层级索引
void arch_pgtbl_set_pte(pte_t* pte, phys_addr_t pa, uint32_t flags); // 设置 PTE 条目
void arch_pgtbl_clear_pte(pte_t* pte); // 清除 PTE 条目
bool arch_pgtbl_pte_valid(pte_t *pte); // 检查 PTE 是否有效
bool arch_pgtbl_pte_is_leaf(pte_t *pte); // 检查 PTE 是否为叶子节点
bool arch_pgtbl_table_is_empty(pte_t *table); // 检查页表是否为空
void arch_pgtbl_flush(void); // 刷新页表缓存（如 TLB）
void arch_pgtbl_switch_to(pgtable_t *pgtbl); // 切换到指定页表

#endif