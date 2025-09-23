.section .text.entry
.globl  _start
_start:

    # 栈设置
    la t0 , __stack_start
    la t1 , __stack_end
    sub t0 , t1 , t0 # t0 = STACK_SIZE
    mul t1 , tp , t0 # t1 = hartid * STACK_SIZE 
    la t2, __stack_end
    sub sp,t2,t1 # 栈底 = _stack_end - hartid * STACK_SIZE

    csrw sscratch, sp  # sscratch存放内核栈指针

    /* jump to init_kernel */
    j init_kernel

_wait_hart:
    wfi
    
.end