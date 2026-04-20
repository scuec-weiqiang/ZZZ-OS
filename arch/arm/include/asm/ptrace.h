#ifndef __ASM_PTRACE_H
#define __ASM_PTRACE_H
#include <os/types.h>
#include <os/config.h>

#include <os/string.h>
#include <os/compiler_attributes.h>
#include <os/bitops.h>

struct task_struct;

#if SYS_BITS == 32
#define USR_MODE	0x00000010
#define SVC_MODE	0x00000013
#define FIQ_MODE	0x00000011
#define IRQ_MODE	0x00000012
#define ABT_MODE	0x00000017
#define HYP_MODE	0x0000001a
#define UND_MODE	0x0000001b
#define SYSTEM_MODE	0x0000001f
#define MODE32_BIT	0x00000010
#define MODE_MASK	0x0000001f

#define V4_PSR_T_BIT	0x00000020	/* >= V4T, but not V7M */
#define PSR_T_BIT	V4_PSR_T_BIT

#define PSR_F_BIT	0x00000040	/* >= V4, but not V7M */
#define PSR_I_BIT	0x00000080	/* >= V4, but not V7M */
#define PSR_A_BIT	0x00000100	/* >= V6, but not V7M */
#define PSR_E_BIT	0x00000200	/* >= V6, but not V7M */
#define PSR_J_BIT	0x01000000	/* >= V5J, but not V7M */
#define PSR_Q_BIT	0x08000000	/* >= V5E, including V7M */
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000
/*
 * Groups of PSR bits
 */
#define PSR_f		0xff000000	/* Flags		*/
#define PSR_s		0x00ff0000	/* Status		*/
#define PSR_x		0x0000ff00	/* Extension		*/
#define PSR_c		0x000000ff	/* Control		*/

struct pt_regs{
    reg_t r[13];
    reg_t sp;
    reg_t lr;
    reg_t pc;
    reg_t cpsr;
};

#define processor_mode(regs) \
	((regs)->cpsr & MODE_MASK)

#define interrupts_enabled(regs) \
	(!((regs)->cpsr & PSR_I_BIT))

#define fast_interrupts_enabled(regs) \
	(!((regs)->cpsr & PSR_F_BIT))

#define isa_mode(regs) \
	((((regs)->cpsr & PSR_J_BIT) >> (__ffs(PSR_J_BIT) - 1)) | \
	 (((regs)->cpsr & PSR_T_BIT) >> (__ffs(PSR_T_BIT))))

#define frame_pointer(regs) ((regs)->r[11])

/* Valid only for Kernel mode traps. */
static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{
	return regs->sp;
}

static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return regs->sp;
}

#define current_pt_regs(void) ({ (struct pt_regs *)			\
		((current_stack_pointer | (THREAD_SIZE - 1)) - 7) - 1;	\
})     

extern void arch_user_enter(struct pt_regs *tf) __noreturn;

#elif SYS_BITS == 64
struct pt_regs {
    unsigned long r[29];
    unsigned long sp;
    unsigned long lr;
    unsigned long pc;
    unsigned long cpsr;
};

static inline void pt_regs_clear(struct pt_regs *tf)
{
    memset(tf, 0, sizeof(*tf));
}

static inline void pt_regs_setup_user(struct pt_regs *tf,
                                        reg_t entry,
                                        reg_t user_sp,
                                        reg_t arg0)
{
    pt_regs_clear(tf);
    tf->pc = entry;
    tf->sp = user_sp;
    tf->r[0] = arg0;
}

static inline void pt_regs_set_retval(struct pt_regs *tf, long val)
{
    tf->r[0] = (reg_t)val;
}

static inline int pt_regs_is_user(const struct pt_regs *tf)
{
    return tf->cpsr != 0;
}

extern void arch_user_enter(struct pt_regs *tf) __noreturn;

#endif
#endif // __ASM_PTRACE_H
