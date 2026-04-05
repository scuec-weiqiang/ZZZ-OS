.section .trap, "ax"
.global vector_table

vector_table:
    b __reset_handler
    b __undef_handler
    b __swi_handler
    b __prefetch_abort_handler
    b __data_abort_handler
    b __reserved_handler
    b __irq_handler
    b __fiq_handler


.global __reset_handler
__reset_handler:
    b .
    // 这里可以添加一些初始化代码，例如设置堆栈指针等

.global __undef_handler
__undef_handler:
    b undef_handler

.global __swi_handler
__swi_handler:
    b swi_handler

.global __prefetch_abort_handler
__prefetch_abort_handler:
    b prefetch_abort_handler

.global __data_abort_handler
__data_abort_handler:
    sub sp, sp, #64
    stmdb sp!, {r0-r12, lr}
    b data_abort_handler


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
    @ bl   irq_handler
    add   sp, sp, #4

    ldmia sp!, {r0}
    msr   spsr_cxsf, r0
    ldmia sp!, {r0-r12, lr}
    subs  pc, lr, #4

.global __fiq_handler
__fiq_handler:      
    b fiq_handler





