/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/barrier.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 21:01:45
 * @LastEditTime: 2025-10-30 21:02:08
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ASM_BARRIER_H
#define _ASM_BARRIER_H

static inline void sfence_vma()
{
    // the zero, zero means flush all TLB entries.
    asm volatile("sfence.vma zero, zero");
}

#endif