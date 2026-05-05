#ifndef __ASM_RISCV64_IO_H
#define __ASM_RISCV64_IO_H

#include <os/types.h>
#include <asm/barrier.h>

#define readb(addr) ({ u8 __v = *(volatile u8 *)(addr); rmb(); __v; })
#define writeb(v, addr) ({ *(volatile u8 *)(addr) = (v); wmb(); })

#define readw(addr) ({ u16 __v = *(volatile u16 *)(addr); rmb(); __v; })
#define writew(v, addr) ({ *(volatile u16 *)(addr) = (v); wmb(); })

#define readl(addr) ({ u32 __v = *(volatile u32 *)(addr); rmb(); __v; })
#define writel(v, addr) ({ *(volatile u32 *)(addr) = (v); wmb(); })

#define readq(addr) ({ u64 __v = *(volatile u64 *)(addr); rmb(); __v; })
#define writeq(v, addr) ({ *(volatile u64 *)(addr) = (v); wmb(); })

#endif
