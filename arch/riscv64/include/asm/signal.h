#ifndef _RISCV64_SIGNAL_H
#define _RISCV64_SIGNAL_H
#include <os/types.h>

static const u32 sigtramp_code[] = {
    0x00d00793, /* li a7, 13          (sigreturn 系统调用号) */
    0x00000073, /* ecall              (触发系统调用)        */
    0x0000006f, /* j .                (无限循环，jal x0, 0) */
};

#endif /* _RISCV64_SIGNAL_H */