.syntax unified
.section .text.startup, "ax"
.align 4
.global _start
.type _start, %function

# _start:
#     mov r4, sp
#     ldr r0, [r4]
#     add r1, r4, #4
#     bl __runtime_entry

# 1:
#     b 1b

.extern main
.extern exit
.extern __libc_init_array

_start:
    ldr r0, [sp]          @ argc
    add r1, sp, #4        @ argv = &argv[0]

    add r2, r1, r0, lsl #2
    add r2, r2, #4        @ envp = &argv[argc + 1]

    bl __libc_init_array
    bl main

    bl exit
1:
    b 1b
