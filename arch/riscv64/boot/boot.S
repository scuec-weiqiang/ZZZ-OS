.section .text.entry
.globl  _start
_start:
    la t0, _early_stack_end
    mv sp, t0  # 设置早期栈指针
    csrw sscratch, sp  # sscratch存放内核栈指针

    j init_kernel

_wait_hart:
    wfi
    
.end