.syntax unified
.text
.align 4

.equ USR_MODE, 0x10
.equ MODE_MASK, 0x1f
.equ PT_REGS_SP,   52
.equ PT_REGS_PC,   60
.equ PT_REGS_CPSR, 64
# struct pt_regs{
#     reg_t r[13];
#     reg_t sp;
#     reg_t lr;
#     reg_t pc;
#     reg_t cpsr;
# };
.global arch_user_enter
.type arch_user_enter, %function
arch_user_enter:
	mov	ip, r0

	ldr	lr, [ip, #PT_REGS_PC]	@ svc lr = user pc
	ldr	r1, [ip, #PT_REGS_CPSR]	@ spsr = target cpsr
	msr	spsr_cxsf, r1

	add	r1, ip, #PT_REGS_SP
	ldmia	r1, {sp, lr}^		@ restore user sp/lr bank
	add	sp, ip, #68		@ drop the bootstrap pt_regs from svc stack
	ldmia	ip, {r0-r12}		@ restore shared registers
	movs	pc, lr			@ switch to user mode and branch


.global ret_from_fork
ret_from_fork:
	blx	sched_tail
	cmp	r5, #0
	beq	1f

	// 是内核线程
	mov	r0, r4
	cpsie i
	blx	r5		// 调用 my_kthread(arg)

	// 处理内核进程执行execve后变成用户进程的情况	
	ldr	r1, [sp, #PT_REGS_CPSR]
	and	r1, r1, #MODE_MASK
	cmp	r1, #USR_MODE
	beq	1f

	bl	do_exit		// 普通内核线程返回后自动退出
1:
	# mov	r0, sp
	# bl	debug_dump_ret_from_fork
	mov	r0, sp
	b	arch_user_enter
	b	1b

.global ret_to_user_from_irq
ret_to_user_from_irq:
	bl	sched_handle_user_return
	mov	r0, sp
	b	arch_user_enter
