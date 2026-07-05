/**
 * @FilePath     : /ZZZ-OS/user_proc/ps/ps.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-05 16:11:59
 * @LastEditTime : 2026-07-05 16:25:00
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/ps.h>

#ifndef SYS_ps
#define SYS_ps 201
#endif

static long zzz_syscall2(long nr, long arg0, long arg1)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a7 asm("a7") = nr;

    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1), "r"(a7)
                  : "memory");

    return a0;
}

static int zzz_ps(struct ps_info *buf, int max)
{
    return (int)zzz_syscall2(SYS_ps, (long)buf, (long)max);
}

static const char *state_name(int s)
{
    switch (s) {
    case 0: return "RUN";
    case 1: return "SLEEP";
    case 2: return "ZOMB";
    case 3: return "DEAD";
    default: return "?";
    }
}

int main(void)
{
    struct ps_info infos[64];
    int n;

    n = zzz_ps(infos, 64);
    if (n < 0) {
        printf("ps: syscall failed: %d\n", n);
        return 1;
    }

    printf("PID  PPID CPU STATE  RQ  COMM             WAIT\n");

    for (int i = 0; i < n; i++) {
        printf("%-4d %-4d %-3d %-6s %-3d %-16s %s\n",
               infos[i].pid,
               infos[i].ppid,
               infos[i].cpu,
               state_name(infos[i].status),
               infos[i].on_rq,
               infos[i].comm,
               infos[i].wait_reason);
    }

    return 0;
}
