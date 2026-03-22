/**
 * @FilePath     : /ZZZ-OS/include/os/mm/pgprot.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-13 18:44:52
 * @LastEditTime : 2026-03-19 17:12:40
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef _MM_PGPROT_H
#define _MM_PGPROT_H
#include <os/bitops.h>

typedef unsigned int pgprot_t;

#define __pgprot(x) ((pgprot_t){ (x) })

#define PROT_NONE          (0)
#define PROT_READ       BIT(0)
#define PROT_WRITE      BIT(1)
#define PROT_EXEC       BIT(2)
#define PROT_USER       BIT(3)

#define PROT_DEVICE     BIT(4)
#define PROT_UNCACHED   BIT(5)

#define PROT_GLOBAL     BIT(6)

#define PAGE_KERNEL \
    __pgprot(PROT_READ | PROT_WRITE  |  PROT_GLOBAL)

#define PAGE_KERNEL_RO \
    __pgprot(PROT_READ | PROT_GLOBAL)

#define PAGE_KERNEL_EXEC \
    __pgprot(PROT_READ | PROT_EXEC | PROT_GLOBAL)

#define PAGE_DEVICE \
    __pgprot(PROT_READ | PROT_WRITE | PROT_DEVICE | PROT_GLOBAL)

#define PAGE_DMA \
    __pgprot(PROT_READ | PROT_WRITE | PROT_UNCACHED | PROT_GLOBAL)

#endif