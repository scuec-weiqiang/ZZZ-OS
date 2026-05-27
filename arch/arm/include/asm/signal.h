#ifndef _ASM_SIGNAL_H
#define _ASM_SIGNAL_H
#include <os/types.h>

#if SYS_BITS == 32
static const u32 sigtramp_code[] = {
    0xe3a0700d, /* mov r7, #13 (sigreturn) */
    0xef000000, /* svc #0 */
    0xeafffffe, /* b . */
};
#endif

#endif /* _ASM_SIGNAL_H */