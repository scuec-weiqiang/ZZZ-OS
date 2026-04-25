.section .trap, "ax"
.align 4
.global vector_table

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
    // 这里可以添加一些初始化代码，例如设置堆栈指针等

.global __undef_handler
__undef_handler:
    b undef_handler

.global __swi_handler
__swi_handler:
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
    mrs r0, spsr
    b prefetch_abort_handler

.global __data_abort_handler
__data_abort_handler:
    sub sp, sp, #64
    stmdb sp!, {r0-r12, lr}
    b data_abort_handler   @ 加载符号地址，链接器会填入最低位 1


.global __reserved_handler
__reserved_handler:
    b reserved_handler


.global __irq_handler
__irq_handler:
    stmdb sp!, {r0-r12, lr}
    mrs   r0, spsr
    stmdb sp!, {r0}

    sub   sp, sp, #4        @ 补齐到 8-byte 对齐
    mov   r0, #0
    ldr   r1, =irq_handler   @ 加载符号地址，链接器会填入最低位 1
    blx   r1

    ldr   r1, =irq_exit
    blx   r1
    
    add   sp, sp, #4

    ldmia sp!, {r0}
    msr   spsr_cxsf, r0
    ldmia sp!, {r0-r12, lr}
    subs  pc, lr, #4

.global __fiq_handler
__fiq_handler:      
    b fiq_handler


