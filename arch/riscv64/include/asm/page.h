/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/page.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 00:03:03
 * @LastEditTime: 2025-10-31 00:03:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ASM_RISCV64_PGTABLE_H
#define _ASM_RISCV64_PGTABLE_H

#define PAGE_SIZE 0x1000    // 页大小4096字节
#define PAGE_SHIFT 12     // 页偏移量
#define PAGE_MASK (~(PAGE_SIZE - 1))   // 页掩码

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#endif
