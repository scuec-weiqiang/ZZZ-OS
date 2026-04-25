.section .rodata.user_mode_smoke, "a", %progbits
.align 2

.global __user_mode_smoke_start
.global __user_mode_smoke_end

__user_mode_smoke_start:
	mov	r7, #1
	svc	#0
	b	.
__user_mode_smoke_end:
