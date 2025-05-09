/*******************************************************************************************
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/clint.h
 * @Description  : 核心本地中断控制器(Core Local Interruptor)头文件 ，用于定时器中断和软件中断的触发。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 16:29:49
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#ifndef CLINT_H
#define CLINT_H

#include "types.h"

#define CLINT_BASE          0x02000000

#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP_BASE         (CLINT_BASE + (0x4000))

#define CLINT_MSIP(hartid)          (CLINT_BASE + 4*(hartid))

#define RELEASE_CORE(hartid)        (*(uint32_t*)CLINT_MSIP(hartid)=1)

__SELF __INLINE uint64_t __clint_mtime_get()
{
    return *(uint64_t*)CLINT_MTIME;
}

__SELF __INLINE void __clint_mtimecmp_set(uint32_t hartid,uint64_t value)
{
    uint64_t *clint_mtimecmp = (uint64_t*)CLINT_MTIMECMP_BASE;
    clint_mtimecmp[hartid] = value;
}

#endif 