/**
 * @FilePath     : /ZZZ-OS/include/os/mm/mm_types.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-13 22:27:02
 * @LastEditTime : 2026-03-13 22:27:03
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_MM_TYPES_H
#define __KERNEL_MM_TYPES_H

#include <os/types.h>
#include <mm/pgtbl_types.h>
#include <mm/vma.h>

struct mm_struct {
    pgtable_t *pgdir;          // 页表根目录

    // 内核不需要用户空间相关的信息
    struct vma vma_list; // 虚拟内存区域链表头
    int vma_count;           // 虚拟内存区域数量
    unsigned long start_brk, brk;
    unsigned long start_stack;
    unsigned long start_code, end_code;
    unsigned long start_data, end_data;
};

#endif /* __KERNEL_MM_TYPES_H__ */