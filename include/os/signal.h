
#ifndef _OS_SIGNAL_H
#define _OS_SIGNAL_H

#include <os/types.h>
#include <asm/ptrace.h>
/*
 * Bits in flags field of signal_struct.
 */
#define SIGNAL_STOP_STOPPED	0x00000001 /* job control stop in effect */
#define SIGNAL_STOP_CONTINUED	0x00000002 /* SIGCONT since WCONTINUED reap */
#define SIGNAL_GROUP_EXIT	0x00000004 /* group exit in progress */
#define SIGNAL_GROUP_COREDUMP	0x00000008 /* coredump in progress */
/*
 * Pending notifications to parent.
 */
#define SIGNAL_CLD_STOPPED	0x00000010
#define SIGNAL_CLD_CONTINUED	0x00000020
#define SIGNAL_CLD_MASK		(SIGNAL_CLD_STOPPED|SIGNAL_CLD_CONTINUED)

#define SIGNAL_UNKILLABLE	0x00000040 /* for init: ignore fatal signals */

#define NSIG 32
#define SIGF 0x53494746

#define SIG_DFL ((void *)0)
#define SIG_IGN ((void *)1)


#define SIGINT      2
#define SIGILL      4
#define SIGSEGV     11
#define SIGKILL     9
#define SIGTERM     15
#define SIGCHLD     17
#define SIGTTIN     21

typedef void (*signalfn_t)(int);

struct k_sigaction {
    signalfn_t handler;
    unsigned long mask;
    int flags;
};

struct signal_frame {
    u32 magic;
    u32 signo;

    struct pt_regs saved_tf;
    unsigned long old_blocked;
};

void send_signal(struct task_struct *t, int sig);

#endif
