/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/platform.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 23:59:39
 * @LastEditTime: 2025-11-14 15:24:22
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef PLATFORM_H
#define PLATFORM_H

#define RAM_SIZE 0x8000000 
#define RAM_BASE 0x80000000

#define SYS_CLOCK_FREQ 10000000

#define MAX_HARTS_NUM 1

#define CLINT_BASE          0x02000000
#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP_BASE         (CLINT_BASE + (0x4000))
#define CLINT_MSIP(hartid)          (CLINT_BASE + 4*(hartid))
#define RELEASE_CORE(hartid)        (*(uint32_t*)CLINT_MSIP(hartid)=1)

#endif