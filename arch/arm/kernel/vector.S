.section .trap, "ax"
.align 4
.global vector_table

.equ USR_MODE, 0x10
.equ SVC_MODE, 0x13
.equ MODE_MASK, 0x1f

vector_table:
    ldr pc, 1f
    ldr pc, 2f
    ldr pc, 3f
    ldr pc, 4f
    ldr pc, 5f
    ldr pc, 6f
    ldr pc, 7f
    ldr pc, 8f

1:  .word __reset_handler
2:  .word __undef_handler
3:  .word __swi_handler
4:  .word __prefetch_abort_handler
5:  .word __data_abort_handler
6:  .word __reserved_handler
7:  .word __irq_handler
8:  .word __fiq_handler


.global __reset_handler
__reset_handler:
    b .
    @ 这里可以添加一些初始化代码，例如设置堆栈指针等

.global __undef_handler
__undef_handler:
    b undef_handler

.global __swi_handler
__swi_handler:
    cpsid i
    sub   sp, sp, #68
    stmia sp, {r0-r12}
    add   r1, sp, #52
    stmia r1, {sp, lr}^
    str   lr, [sp, #60]
    mrs   r0, spsr
    str   r0, [sp, #64]
    mov   r0, sp
    bl    swi_handler
    ldr   r0, [sp, #64]
    msr   spsr_cxsf, r0
    add   r1, sp, #52
    ldmia r1, {sp, lr}^
    ldr   lr, [sp, #60]
    ldmia sp, {r0-r12}
    add   sp, sp, #68
    movs  pc, lr

.global __prefetch_abort_handler
__prefetch_abort_handler:
    stmdb sp!, {r0-r12, lr}
    mrs   r0, spsr
    stmdb sp!, {r0}
    sub   sp, sp, #4
    bl    prefetch_abort_handler
    add   sp, sp, #4
    cmp   r0, #0
    bne   .
    ldmia sp!, {r0}
    msr   spsr_cxsf, r0
    ldmia sp!, {r0-r12, lr}
    subs  pc, lr, #4

.global __data_abort_handler
__data_abort_handler:
    stmdb sp!, {r0-r12, lr}
    mrs   r0, spsr
    stmdb sp!, {r0}
    sub   sp, sp, #4
    bl    data_abort_handler
    add   sp, sp, #4
    cmp   r0, #0
    bne   .
    ldmia sp!, {r0}
    msr   spsr_cxsf, r0
    ldmia sp!, {r0-r12, lr}
    subs  pc, lr, #8


.global __reserved_handler
__reserved_handler:
    b reserved_handler


.global __irq_handler
__irq_handler:
    cpsid i
    sub   sp, sp, #68
    stmia sp, {r0-r12}
    add   r1, sp, #52
    stmia r1, {sp, lr}^
    str   lr, [sp, #60]
    mrs   r0, spsr
    str   r0, [sp, #64]

    mov   r0, #0
    ldr   r1, =irq_handler 
    blx   r1

    ldr   r0, [sp, #64]
    ldr   r1, =irq_exit
    blx   r1

    ldr   r0, [sp, #64]
    and   r0, r0, #MODE_MASK
    cmp   r0, #USR_MODE
    beq   1f

    ldr   r0, [sp, #64]
    msr   spsr_cxsf, r0
    ldr   lr, [sp, #60]
    ldmia sp, {r0-r12}
    add   sp, sp, #68
    subs  pc, lr, #4

1:
    mov   r0, sp
    ldr   r1, =irq_prepare_user_return 
    blx   r1 @因为irq和scv模式下不是一个栈，这个函数把irq保存的内容复制到svc模式下的栈中
    cps   #SVC_MODE @切换到svc模式，irq_prepare_user_return函数已经把svc模式下的栈准备好了
    mov   sp, r0 @切换到svc模式后，就可以正常恢复
    b     ret_to_user_from_irq

.global __fiq_handler
__fiq_handler:      
    b fiq_handler
