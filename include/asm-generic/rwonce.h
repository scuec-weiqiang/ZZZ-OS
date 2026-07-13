/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Prevent the compiler from merging or refetching reads or writes. The
 * compiler is also forbidden from reordering successive instances of
 * READ_ONCE and WRITE_ONCE, but only when the compiler is aware of some
 * particular ordering. One way to make the compiler aware of ordering is to
 * put the two invocations of READ_ONCE or WRITE_ONCE in different C
 * statements.
 *
 * These two macros will also work on aggregate data types like structs or
 * unions.
 *
 * Their two major use cases are: (1) Mediating communication between
 * process-level code and irq/NMI handlers, all running on the same CPU,
 * and (2) Ensuring that the compiler does not fold, spindle, or otherwise
 * mutilate accesses that either do not require ordering or that interact
 * with an explicit memory barrier or atomic instruction that provides the
 * required ordering.
 */
#ifndef __ASM_GENERIC_RWONCE_H
#define __ASM_GENERIC_RWONCE_H

#ifndef __ASSEMBLY__

#include <os/compiler.h>
/*
 * Use __READ_ONCE() instead of READ_ONCE() if you do not require any
 * atomicity. Note that this may result in tears!
 */

#define READ_ONCE(x)	(*(const volatile __unqual_scalar_typeof(x) *)&(x))

#define WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)


#endif /* __ASSEMBLY__ */
#endif	/* __ASM_GENERIC_RWONCE_H */
