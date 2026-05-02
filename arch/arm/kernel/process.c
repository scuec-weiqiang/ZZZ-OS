/*
 *  linux/arch/arm/kernel/process.c
 *
 *  Copyright (C) 1996-2000 Russell King - Converted to ARM.
 *  Original Copyright (C) 1995  Linus Torvalds
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdarg.h>
#include <os/printk.h>
#include <os/sched.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>
#include <asm/process.h>
#include <asm/stacktrace.h>


static const char *processor_modes[]  = {
  "USER_26", "FIQ_26" , "IRQ_26" , "SVC_26" , "UK4_26" , "UK5_26" , "UK6_26" , "UK7_26" ,
  "UK8_26" , "UK9_26" , "UK10_26", "UK11_26", "UK12_26", "UK13_26", "UK14_26", "UK15_26",
  "USER_32", "FIQ_32" , "IRQ_32" , "SVC_32" , "UK4_32" , "UK5_32" , "MON_32" , "ABT_32" ,
  "UK8_32" , "UK9_32" , "HYP_32", "UND_32" , "UK12_32", "UK13_32", "UK14_32", "SYS_32"
};

static const char *isa_modes[]  = {
  "ARM" , "Thumb" , "Jazelle", "ThumbEE"
};

static void dump_reg_words(const char *tag, struct pt_regs *regs) {
    unsigned long *p = (unsigned long *)regs;
    int i;

    if (!regs) {
        dprintk("%s: regs=NULL\n", tag);
        return;
    }

    dprintk("%s: regs=%x words=", tag, regs);
    for (i = 0; i < 8; i++) {
        printk(" %x", p[i]);
    }
    printk("\n");
}

void debug_dump_ret_from_fork(struct pt_regs *regs) {
    if (!regs) {
        dprintk("fork: ret_from_fork regs=NULL\n");
        return;
    }

    dprintk("fork: ret_from_fork pid=%d regs=%x {r0=%x r1=%x r7=%x r11=%x sp=%x lr=%x pc=%x cpsr=%x}\n",
            current ? current->pid : -1, regs,
            regs->r[0], regs->r[1], regs->r[7], regs->r[11],
            regs->sp, regs->lr, regs->pc, regs->cpsr);
    dump_reg_words("fork: ret_from_fork words", regs);
}

void show_regs(struct pt_regs *regs) {
	unsigned long flags;
	char buf[64];

    printk("-----------------------------------");

	printk("PC is at %s\n", regs->pc);
	printk("LR is at %s\n", regs->lr);
	printk("pc : [<%xu>]    lr : [<%xu>]    psr: %xu\n" "sp : %xu \n",
		regs->pc, regs->lr, regs->cpsr,regs->sp);
	printk("r10: %xu  r9 : %xu  r8 : %xu\n",
		regs->r[10], regs->r[9],
		regs->r[8]);
	printk("r7 : %xu  r6 : %xu  r5 : %xu  r4 : %xu\n",
		regs->r[7], regs->r[6],
		regs->r[5], regs->r[4]);
	printk("r3 : %xu  r2 : %xu  r1 : %xu  r0 : %xu\n",
		regs->r[3], regs->r[2],
		regs->r[1], regs->r[0]);

	flags = regs->cpsr;
	buf[0] = flags & PSR_N_BIT ? 'N' : 'n';
	buf[1] = flags & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = flags & PSR_C_BIT ? 'C' : 'c';
	buf[3] = flags & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';

	printk("Flags: %s  IRQs o%s  FIQs o%s  Mode %s  ISA %s \n",
		buf, interrupts_enabled(regs) ? "n" : "ff",
		fast_interrupts_enabled(regs) ? "n" : "ff",
		processor_modes[processor_mode(regs)],
		isa_modes[isa_mode(regs)]);
}

extern void ret_from_fork(void);

void start_thread(struct pt_regs *regs, unsigned long entry, unsigned long sp) {
    memset(regs, 0, sizeof(*regs));
    regs->pc = entry;
    regs->sp = sp;
    regs->cpsr = USR_MODE;
}

int setup_kthread_context(int (*fn)(void *), void *arg, struct task_struct *p) {
	struct thread_info *thread = task_thread_info(p);
	struct pt_regs *childregs = task_pt_regs(p);

	memset(&thread->cpu_context, 0, sizeof(struct cpu_context_save));

	memset(childregs, 0, sizeof(struct pt_regs));
	thread->cpu_context.r4 = (unsigned long)arg;
	thread->cpu_context.r5 = (unsigned long)fn;
	childregs->cpsr = SVC_MODE;
	
	thread->cpu_context.pc = (unsigned long)ret_from_fork;
	thread->cpu_context.sp = (unsigned long)childregs;

	return 0;
}

int setup_uthread_context(struct task_struct *p) {
	struct thread_info *thread = task_thread_info(p);
	struct pt_regs *childregs = task_pt_regs(p);

	memset(&thread->cpu_context, 0, sizeof(struct cpu_context_save));

	*childregs = *current_pt_regs();
	childregs->r[0] = 0;
	
	thread->cpu_context.pc = (unsigned long)ret_from_fork;
	thread->cpu_context.sp = (unsigned long)childregs;

	return 0;
}


// unsigned long get_wchan(struct task_struct *p)
// {
// 	struct stackframe frame;
// 	unsigned long stack_page;
// 	int count = 0;
// 	if (!p || p == current || p->status == TASK_RUNNING)
// 		return 0;

// 	frame.fp = thread_saved_fp(p);
// 	frame.sp = thread_saved_sp(p);
// 	frame.lr = 0;			/* recovered from the stack */
// 	frame.pc = thread_saved_pc(p);
// 	stack_page = (unsigned long)task_stack_page(p);
// 	do {
// 		if (frame.sp < stack_page ||
// 		    frame.sp >= stack_page + THREAD_SIZE ||
// 		    unwind_frame(&frame) < 0)
// 			return 0;
// 		if (!in_sched_functions(frame.pc))
// 			return frame.pc;
// 	} while (count ++ < 16);
// 	return 0;
// }

// unsigned long arch_randomize_brk(struct mm_struct *mm)
// {
// 	unsigned long range_end = mm->brk + 0x02000000;
// 	return randomize_range(mm->brk, range_end, 0) ? : mm->brk;
// }

// #ifdef CONFIG_MMU
// #ifdef CONFIG_KUSER_HELPERS
// /*
//  * The vectors page is always readable from user space for the
//  * atomic helpers. Insert it into the gate_vma so that it is visible
//  * through ptrace and /proc/<pid>/mem.
//  */
// static struct vm_area_struct gate_vma = {
// 	.vm_start	= 0xffff0000,
// 	.vm_end		= 0xffff0000 + PAGE_SIZE,
// 	.vm_flags	= VM_READ | VM_EXEC | VM_MAYREAD | VM_MAYEXEC,
// };

// static int __init gate_vma_init(void)
// {
// 	gate_vma.vm_page_prot = PAGE_READONLY_EXEC;
// 	return 0;
// }
// arch_initcall(gate_vma_init);

// struct vm_area_struct *get_gate_vma(struct mm_struct *mm)
// {
// 	return &gate_vma;
// }

// int in_gate_area(struct mm_struct *mm, unsigned long addr)
// {
// 	return (addr >= gate_vma.vm_start) && (addr < gate_vma.vm_end);
// }

// int in_gate_area_no_mm(unsigned long addr)
// {
// 	return in_gate_area(NULL, addr);
// }
// #define is_gate_vma(vma)	((vma) == &gate_vma)
// #else
// #define is_gate_vma(vma)	0
// #endif

// const char *arch_vma_name(struct vm_area_struct *vma)
// {
// 	return is_gate_vma(vma) ? "[vectors]" : NULL;
// }

// /* If possible, provide a placement hint at a random offset from the
//  * stack for the sigpage and vdso pages.
//  */
// static unsigned long sigpage_addr(const struct mm_struct *mm,
// 				  unsigned int npages)
// {
// 	unsigned long offset;
// 	unsigned long first;
// 	unsigned long last;
// 	unsigned long addr;
// 	unsigned int slots;

// 	first = PAGE_ALIGN(mm->start_stack);

// 	last = TASK_SIZE - (npages << PAGE_SHIFT);

// 	/* No room after stack? */
// 	if (first > last)
// 		return 0;

// 	/* Just enough room? */
// 	if (first == last)
// 		return first;

// 	slots = ((last - first) >> PAGE_SHIFT) + 1;

// 	offset = get_random_int() % slots;

// 	addr = first + (offset << PAGE_SHIFT);

// 	return addr;
// }

// static struct page *signal_page;
// extern struct page *get_signal_page(void);

// static const struct vm_special_mapping sigpage_mapping = {
// 	.name = "[sigpage]",
// 	.pages = &signal_page,
// };

// int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
// {
// 	struct mm_struct *mm = current->mm;
// 	struct vm_area_struct *vma;
// 	unsigned long npages;
// 	unsigned long addr;
// 	unsigned long hint;
// 	int ret = 0;

// 	if (!signal_page)
// 		signal_page = get_signal_page();
// 	if (!signal_page)
// 		return -ENOMEM;

// 	npages = 1; /* for sigpage */
// 	npages += vdso_total_pages;

// 	down_write(&mm->mmap_sem);
// 	hint = sigpage_addr(mm, npages);
// 	addr = get_unmapped_area(NULL, hint, npages << PAGE_SHIFT, 0, 0);
// 	if (IS_ERR_VALUE(addr)) {
// 		ret = addr;
// 		goto up_fail;
// 	}

// 	vma = _install_special_mapping(mm, addr, PAGE_SIZE,
// 		VM_READ | VM_EXEC | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC,
// 		&sigpage_mapping);

// 	if (IS_ERR(vma)) {
// 		ret = PTR_ERR(vma);
// 		goto up_fail;
// 	}

// 	mm->context.sigpage = addr;

// 	/* Unlike the sigpage, failure to install the vdso is unlikely
// 	 * to be fatal to the process, so no error check needed
// 	 * here.
// 	 */
// 	install_vdso(mm, addr + PAGE_SIZE);

//  up_fail:
// 	up_write(&mm->mmap_sem);
// 	return ret;
// }
// #endif
