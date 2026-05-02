/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/interrupt_handler.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-18 16:16:46
 * @LastEditTime : 2026-03-26 19:50:42
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/types.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/syscall.h>
#include <os/mm.h>
#include <os/stacktrace.h>
#include <asm/irq.h>
#include <asm/ptrace.h>
#include <mm/page_fault.h>

extern void _puts(char *s);

static inline u32 read_dfar(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c6,c0,0":"=r"(v));
    return v;
}

static inline u32 read_dfsr(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c5,c0,0":"=r"(v));
    return v;
}

static inline u32 read_ttbr0(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c2,c0,0":"=r"(v));
    return v;
}

static inline u32 read_ttbr1(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c2,c0,1":"=r"(v));
    return v;
}

static inline u32 read_ttbcr(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c2,c0,2":"=r"(v));
    return v;
}

static inline u32 read_dacr(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c3,c0,0":"=r"(v));
    return v;
}

static inline u32 read_ifar(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c6,c0,2":"=r"(v));
    return v;
}

static inline u32 read_ifsr(void)
{
    u32 v;
    asm volatile("mrc p15,0,%0,c5,c0,1":"=r"(v));
    return v;
}
static void exception(char *s) {
    printk("%s exception\n", s);
    while (1) {
    }
}

static inline u32 arm_fault_status(u32 fsr)
{
    return (fsr & 0xF) | ((fsr >> 6) & 0x10);
}

static inline int arm_fault_is_translation(u32 fs)
{
    return fs == 0x5 || fs == 0x7;
}

static inline int arm_fault_is_user(unsigned long spsr)
{
    return (spsr & MODE_MASK) == USR_MODE;
}

static void dump_abort_stack(void)
{
    dump_stack();
}

void reset_handler(void)
{
    exception("reset");
}

void reserved_handler(void)
{
    exception("reserved");
}

void undef_handler(void)
{
   exception("undef");
}

void swi_handler(struct pt_regs *regs)
{
    // local_irq_disable();
    // printk("swi_handler called\n");
    if (regs == NULL) {
        return;
    }

    do_syscall(regs);
}


int prefetch_abort_handler(unsigned long spsr) {
    u32 ifar = read_ifar();
    u32 ifsr = read_ifsr();
    u32 fs = arm_fault_status(ifsr);

    if (arm_fault_is_user(spsr) && arm_fault_is_translation(fs) && current->mm != NULL) {
        if (do_page_fault(current->mm, ifar, PROT_USER | PROT_EXEC) == 0) {
            return 0;
        }
    }

    printk("PREFETCH ABORT ifar=%xu ifsr=%xu fs=%xu spsr=%xu mode=%xu\n",
           ifar, ifsr, fs, spsr, spsr & 0x1f);
    dump_abort_stack();
    exception("prefetch");
    return -1;
}

void irq_exit(unsigned long spsr) {
    (void)spsr;
    irq_run_deferred_works();
}

struct pt_regs *irq_prepare_user_return(struct pt_regs *irq_regs) {
    struct task_struct *task = this_rq()->curr;
    struct pt_regs *user_regs;

    if (task == NULL || irq_regs == NULL) {
        return NULL;
    }

    user_regs = task_pt_regs(task);
    *user_regs = *irq_regs;
    user_regs->pc -= 4;
    return user_regs;
}

reg_t irq_handler(reg_t ctx) {
    if (handle_arch_irq) {
        return handle_arch_irq(&ctx);
    } else {
        printk("No arch irq handler registered\n");
        exception("irq");
        return ctx;
    }
        
}

void fiq_handler(void) {
    exception("fiq");
}

int data_abort_handler(unsigned long spsr) {
    u32 addr = read_dfar();
    u32 stat = read_dfsr();
    u32 fs = arm_fault_status(stat);
    u32 ttbr0 = read_ttbr0();
    u32 ttbr1 = read_ttbr1();
    u32 ttbcr = read_ttbcr();
    u32 dacr = read_dacr();

    if (arm_fault_is_user(spsr) && arm_fault_is_translation(fs) && current->mm != NULL) {
        if (do_page_fault(current->mm, addr, PROT_USER | PROT_READ | PROT_WRITE) == 0) {
            return 0;
        }
    }

    printk("DATA ABORT ");
    printk("addr=%xu stat=%xu fs=%xu spsr=%xu mode=%xu\n",
           addr, stat, fs, spsr, spsr & 0x1f);
    printk("TTBR0=%xu TTBR1=%xu TTBCR=%xu DACR=%xu\n",
           ttbr0, ttbr1, ttbcr, dacr);
    dump_abort_stack();

    exception("data abort");
    return -1;
}
