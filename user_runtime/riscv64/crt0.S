.section .text.startup, "ax"
.align 2
.global _start
.type _start, @function

.extern main
.extern exit
.extern __libc_init_array
.extern environ

_start:
    # la gp, __global_pointer$

    ld a0, 0(sp)          /* argc */
    addi a1, sp, 8        /* argv */
    slli t0, a0, 3        /* argc * sizeof(char *) */
    add a2, a1, t0        /* &argv[argc] */
    addi a2, a2, 8        /* envp = &argv[argc + 1] */

    la t1, environ
    sd a2, 0(t1)

    mv s0, a0
    mv s1, a1
    mv s2, a2

    mv a0, s0
    mv a1, s1
    mv a2, s2
    call __libc_init_array

    mv a0, s0
    mv a1, s1
    mv a2, s2
    call main

    call exit
1:
    j 1b
