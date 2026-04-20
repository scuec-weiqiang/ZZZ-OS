#ifndef __ASM_BITOPS_H
#define __ASM_BITOPS_H

#include <os/types.h>


static inline int constant_fls(int x) {
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

static inline unsigned int __clz(unsigned int x) {
	unsigned int ret;

	asm("clz\t%0, %1" : "=r" (ret) : "r" (x));

	return ret;
}

/*
 * fls() returns zero if the input is zero, otherwise returns the bit
 * position of the last set bit, where the LSB is 1 and MSB is 32.
 */
static inline int fls(int x) {
	if (__builtin_constant_p(x))
	       return constant_fls(x);

	return 32 - __clz(x);
}

/*
 * __fls() returns the bit position of the last bit set, where the
 * LSB is 0 and MSB is 31.  Zero input is undefined.
 */
static inline unsigned long __fls(unsigned long x) {
	return fls(x) - 1;
}

/*
 * ffs() returns zero if the input was zero, otherwise returns the bit
 * position of the first set bit, where the LSB is 1 and MSB is 32.
 */
static inline int ffs(int x) {
	return fls(x & -x);
}

/*
 * __ffs() returns the bit position of the first bit set, where the
 * LSB is 0 and MSB is 31.  Zero input is undefined.
 */
static inline unsigned long __ffs(unsigned long x) {
	return ffs(x) - 1;
}

#define ffz(x) __ffs( ~(x) )

#endif