#ifndef __KERNEL_KVA_H
#define __KERNEL_KVA_H

#include <os/config.h>
#include <os/types.h>

#if SYS_BITS == 64

#define KERNEL_PA_BASE      0x80200000
#define KERNEL_VA_BASE      0xffffffffc0200000
#define KERNEL_MMIO_BASE    0xfffffffff0000000

#elif SYS_BITS == 32

#define KERNEL_PA_BASE      0x80000000
#define KERNEL_VA_BASE      0xc0000000
#define KERNEL_MMIO_BASE    0xf0000000

#endif

#define KERNEL_VA(pa) ((virt_addr_t)(KERNEL_VA_BASE + ((uintptr_t)(pa)) - KERNEL_PA_BASE))
#define KERNEL_PA(va) ((phys_addr_t)(va) - KERNEL_VA_BASE + KERNEL_PA_BASE)


#endif // __KERNEL_KVA_H