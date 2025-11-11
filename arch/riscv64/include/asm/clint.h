#ifndef __ASM_CLINT_H__
#define __ASM_CLINT_H__

// #include <os/irq_chip.h>
#define RISCV64_CLINT_IRQ_BASE 0
#define RISCV64_CLINT_IRQ_COUNT 16

extern struct irq_chip_ops riscv64_clint_chip_ops;

#endif // __ASM_CLINT_H__
