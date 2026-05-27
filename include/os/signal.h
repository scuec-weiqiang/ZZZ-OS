
#ifndef _OS_SIGNAL_H
#define _OS_SIGNAL_H

#include <os/types.h>
#include <asm/ptrace.h>

#define NSIG 32
#define SIGF 0x53494746

#define SIG_DFL ((void *)0)
#define SIG_IGN ((void *)1)


#define SIGINT      2
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
