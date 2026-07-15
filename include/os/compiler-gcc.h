/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _OS_COMPILER_GCC_H
#define _OS_COMPILER_GCC_H

#ifndef __GNUC__
#error "compiler-gcc.h requires GCC"
#endif

/* Keep compiler-specific builtins here. Generic attributes live in
 * compiler_attributes.h and generic helpers live in compiler.h.
 */
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + \
		     __GNUC_PATCHLEVEL__)

#define __compiler_offsetof(type, member) \
	__builtin_offsetof(type, member)

#ifndef barrier_data
#define barrier_data(ptr) \
	__asm__ __volatile__("" : : "r"(ptr) : "memory")
#endif

#ifndef asm_volatile_goto
#define asm_volatile_goto(x...) \
	do { asm goto(x); asm (""); } while (0)
#endif

#ifndef uninitialized_var
#define uninitialized_var(x) x = x
#endif

#ifndef __compiletime_object_size
#define __compiletime_object_size(obj) __builtin_object_size(obj, 0)
#endif

#endif /* _OS_COMPILER_GCC_H */
