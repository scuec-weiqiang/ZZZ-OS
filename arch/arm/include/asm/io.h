/*
 *  arch/arm/include/asm/io.h
 *
 *  Copyright (C) 1996-2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modifications:
 *  16-Sep-1996	RMK	Inlined the inx/outx functions & optimised for both
 *			constant addresses and variable addresses.
 *  04-Dec-1997	RMK	Moved a lot of this stuff to the new architecture
 *			specific IO header files.
 *  27-Mar-1999	PJB	Second parameter of memcpy_toio is const..
 *  04-Apr-1999	PJB	Added check_signature.
 *  12-Dec-1999	RMK	More cleanups
 *  18-Jun-2000 RMK	Removed virt_to_* and friends definitions
 *  05-Oct-2004 BJD     Moved memory string functions to use void __iomem
 */
#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#include <os/types.h>
#include <asm-generic/barrier.h>


 /* 8bit 读写 */
 #define readb(addr)	({ u8  __v = *(volatile u8  *)(addr); rmb(); __v; })
 #define writeb(v, addr)	({ volatile u8  __a = *(volatile u8  *)(addr); (void)__a; *(volatile u8  *)(addr) = (v); wmb(); })
 
 /* 16bit 读写 */
 #define readw(addr)	({ u16 __v = *(volatile u16 *)(addr); rmb(); __v; })
 #define writew(v, addr)	({ volatile u16 __a = *(volatile u16 *)(addr); (void)__a; *(volatile u16 *)(addr) = (v); wmb(); })
 
 /* 32bit 读写 */
 #define readl(addr)	({ u32 __v = *(volatile u32 *)(addr); rmb(); __v; })
 #define writel(v, addr)	({ volatile u32 __a = *(volatile u32 *)(addr); (void)__a; *(volatile u32 *)(addr) = (v); wmb(); })
 

#endif	/* __ASM_ARM_IO_H */
