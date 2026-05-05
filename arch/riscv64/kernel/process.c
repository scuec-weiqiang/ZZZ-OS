#include <os/printk.h>
#include <os/sched.h>
#include <os/string.h>
#include <asm/process.h>

extern void ret_from_fork(void);

void show_regs(struct pt_regs *regs)
{
    if (regs == NULL) {
        return;
    }

    printk("pc : %xu status: %xu cause: %xu tval: %xu\n",
           regs->sepc, regs->sstatus, regs->scause, regs->stval);
    printk("ra : %xu sp: %xu gp: %xu tp: %xu\n",
           regs->ra, regs->sp, regs->gp, regs->tp);
    printk("a0 : %xu a1: %xu a2: %xu a3: %xu\n",
           regs->a0, regs->a1, regs->a2, regs->a3);
    printk("a4 : %xu a5: %xu a6: %xu a7: %xu\n",
           regs->a4, regs->a5, regs->a6, regs->a7);
    printk("s0 : %xu s1: %xu s2: %xu s3: %xu\n",
           regs->s0, regs->s1, regs->s2, regs->s3);
    printk("s4 : %xu s5: %xu s6: %xu s7: %xu\n",
           regs->s4, regs->s5, regs->s6, regs->s7);
    printk("s8 : %xu s9: %xu s10: %xu s11: %xu\n",
           regs->s8, regs->s9, regs->s10, regs->s11);
}

void start_thread(struct pt_regs *regs, unsigned long entry, unsigned long sp)
{
    pt_regs_setup_user(regs, entry, sp, 0);
}

int setup_kthread_context(int (*fn)(void *), void *arg, struct task_struct *p)
{
    struct thread_info *thread = task_thread_info(p);
    struct pt_regs *childregs = task_pt_regs(p);

    memset(&thread->cpu_context, 0, sizeof(thread->cpu_context));
    memset(childregs, 0, sizeof(*childregs));

    childregs->sstatus = SSTATUS_SPP | SSTATUS_SPIE;
    thread->cpu_context.s0 = (unsigned long)arg;
    thread->cpu_context.s1 = (unsigned long)fn;
    thread->cpu_context.ra = (unsigned long)ret_from_fork;
    thread->cpu_context.sp = (unsigned long)childregs;

    return 0;
}

int setup_uthread_context(struct task_struct *p)
{
    struct thread_info *thread = task_thread_info(p);
    struct pt_regs *childregs = task_pt_regs(p);

    memset(&thread->cpu_context, 0, sizeof(thread->cpu_context));

    *childregs = *current_pt_regs();
    childregs->a0 = 0;
    thread->cpu_context.ra = (unsigned long)ret_from_fork;
    thread->cpu_context.sp = (unsigned long)childregs;

    return 0;
}
