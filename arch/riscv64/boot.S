.section .text.entry
.globl  _start
_start:
    /* set sp to stack_end */
    la sp, __stack_end
    /* jump to init_kernel */
    j init_kernel

.end