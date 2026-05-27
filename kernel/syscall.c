#include <asm/ptrace.h>
#include <os/sched.h>
#include <os/errno.h>
#include <os/syscall.h>
#include <os/syscall_num.h>

const syscall_fn_t syscall_table[SYSCALL_MAX] = {
#define X(nr, name) [nr] = sys_##name,
    SYSCALL_LIST
#undef X
};

void do_syscall(struct pt_regs *ctx) {
    long ret = -ENOSYS;
    u32 nr;

    if (ctx == NULL)
        return;

    nr = ctx->r[7];
    // dprintk("syscall: nr=%u\n", nr);
    current_thread_info()->syscall = nr;

    if (nr < SYSCALL_MAX && syscall_table[nr] != NULL)
        ret = syscall_table[nr](ctx);

    if (nr == SYSCALL_sigreturn) {

    } else {
        ctx->r[0] = (reg_t)ret;
    }
        
}
