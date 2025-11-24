/**
 * @FilePath: /ZZZ-OS/include/os/pfn.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-17 19:54:47
 * @LastEditTime: 2025-11-20 21:16:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PFN_H_
#define __KERNEL_PFN_H_

typedef unsigned long pfn_t;

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))   // 页掩码

#define SIZE_4K  (4 * 1024)
#define SIZE_1M (1024 * 1024)
#define SIZE_2M (2 * SIZE_1M)
#define SIZE_1G (1024 * SIZE_1M)


#define phys_to_pfn(pa)  ((pa) >> PAGE_SHIFT)
#define pfn_to_phys(pfn) ((pfn) << PAGE_SHIFT)

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

#define PAGE_ALIGN(addr)  ALIGN_UP(addr, PAGE_SIZE)

#endif
