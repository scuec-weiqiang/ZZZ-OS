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

struct mm_struct {
    pgtable_t *pgdir;          // 页表根目录

    // 内核不需要用户空间相关的信息
    struct list_head vma_list;      // 进程的虚拟内存区域链表
    int vma_count;           // 虚拟内存区域数量
};

#endif /* __KERNEL_MM_TYPES_H__ */