/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/platform.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 23:59:39
 * @LastEditTime: 2025-09-21 13:33:31
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef PLATFORM_H
#define PLATFORM_H

#define RAM_SIZE 0x8000000 
#define RAM_BASE 0x80000000

enum hart_id
{
    HART_0,
    HART_1,
    MAX_HARTS_NUM,
};

#define SYS_CLOCK_FREQ 10000000


#define PLIC_BASE                    0x0c000000
#define PLIC_PRIORITY_BASE            (PLIC_BASE + (0x0000))
#define PLIC_PENDING_BASE             (PLIC_BASE + (0x1000))
#define PLIC_INT_EN_BASE              (PLIC_BASE + (0x2000))
#define PLIC_INT_THRSHOLD_BASE        (PLIC_BASE + (0x200000))
#define PLIC_CLAIM_BASE               (PLIC_BASE + (0x200004))
#define PLIC_COMPLETE_BASE            (PLIC_BASE + (0x200004))

#define UART0_IRQN 10
#define UART_BASE  0x10000000

#define CLINT_BASE          0x02000000
#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP_BASE         (CLINT_BASE + (0x4000))
#define CLINT_MSIP(hartid)          (CLINT_BASE + 4*(hartid))
#define RELEASE_CORE(hartid)        (*(uint32_t*)CLINT_MSIP(hartid)=1)

#define REAL_TIME_BASE 0x50000000

#endif