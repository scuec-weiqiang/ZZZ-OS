#ifndef _SYS_PS_H
#define _SYS_PS_H

struct ps_info {
    int pid;
    int cpu;
    int status;
    int on_rq;
    int need_resched;
    unsigned int flags;
};

#define PS_FLAG_KTHREAD 0x00200000U

#endif
