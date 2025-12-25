.section .text.entry
.globl  _start
_start:
    la t0, _early_stack_end
    mv sp, t0  # 设置早期栈指针
    csrw sscratch, sp  # sscratch存放内核栈指针

    la t0, init_kernel
    addi t0, t0, 0xffffffffc0000000  # 内核地址转换
    jr t0

_wait_hart:
    wfi
    
.end