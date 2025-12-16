#ifndef __KERNEL_VA_H
#define __KERNEL_VA_H

#define KERNEL_PA_BASE 0x80000000
// #define KERNEL_VA_BASE 0xffffffffc0000000
#define KERNEL_VA_BASE 0x80000000
// #define KERNEL_VA_START 0xffffffffc0200000
#define KERNEL_VA_START 0x80400000
#define KERNEL_VA(pa) (KERNEL_VA_BASE + ((uint64_t)(pa)) - KERNEL_PA_BASE)
#define KERNEL_PA(va) ((uint64_t)(va) - KERNEL_VA_BASE + KERNEL_PA_BASE)


#endif // __KERNEL_VA_H