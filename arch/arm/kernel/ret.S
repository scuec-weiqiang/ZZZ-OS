.syntax unified
.text
.align 4


.global ret_from_fork
ret_from_fork:
	blx	sched_tail
	cmp	r5, #0
	beq	1f

	// 是内核线程
	mov	r0, r4
	blx	r5		// 调用 my_kthread(arg)
	mov	r0, r0		// 返回值 = exit code
	bl	do_exit		// 自动退出
1:
	b 1b