.syntax unified
.section .text.startup, "ax"
.align 4
.global _start
.type _start, %function

.equ SYS_sigreturn,13

.global __sigreturn_trampoline
__sigreturn_trampoline:
    mov r7, #SYS_sigreturn
    svc #0
1:
    b 1b

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
.extern environ

.equ CRT0_DEBUG, 0
.equ SYS_write, 4

.if CRT0_DEBUG
debug_write:
    push {r4, r7, lr}
    mov  r2, r1
    mov  r1, r0
    mov  r0, #1
    mov  r7, #SYS_write
    svc  #0
    pop  {r4, r7, lr}
    bx   lr

debug_write_cstr:
    push {r1-r3, lr}
    mov  r1, r0
1:
    ldrb r2, [r1]
    cmp  r2, #0
    beq  2f
    add  r1, r1, #1
    b    1b
2:
    sub  r1, r1, r0
    bl   debug_write
    pop  {r1-r3, lr}
    bx   lr

debug_write_hex32:
    push {r1-r6, lr}
    ldr  r1, =hex_buf
    mov  r2, #0x30
    strb r2, [r1]
    mov  r2, #0x78
    strb r2, [r1, #1]
    mov  r3, #8
    add  r4, r1, #2
    ldr  r6, =hex_digits
1:
    mov  r5, r0, lsr #28
    ldrb r5, [r6, r5]
    strb r5, [r4], #1
    lsl  r0, r0, #4
    subs r3, r3, #1
    bne  1b
    mov  r5, #0x0a
    strb r5, [r4]
    ldr  r0, =hex_buf
    mov  r1, #11
    bl   debug_write
    pop  {r1-r6, lr}
    bx   lr
.endif

# _start:
#     ldr r0, [sp]          @ argc
#     add r1, sp, #4        @ argv = &argv[0]
#     add r2, r1, r0, lsl #2
#     add r2, r2, #4        @ envp = &argv[argc + 1]
#     mov r5, r0
#     mov r6, r1
#     mov r7, r2

# .if CRT0_DEBUG
#     mov r4, sp

#     ldr r0, =msg_crt0_enter
#     bl  debug_write_cstr
#     mov r0, r4
#     bl  debug_write_hex32

#     ldr r0, =msg_argc
#     bl  debug_write_cstr
#     mov r0, r5
#     bl  debug_write_hex32

#     ldr r0, =msg_argv
#     bl  debug_write_cstr
#     mov r0, r6
#     bl  debug_write_hex32

#     ldr r0, =msg_envp
#     bl  debug_write_cstr
#     mov r0, r7
#     bl  debug_write_hex32

#     cmp r5, #0
#     beq 1f
#     ldr r0, [r6]
#     cmp r0, #0
#     beq 1f
#     ldr r1, =msg_argv0
#     mov r2, r0
#     mov r0, r1
#     bl  debug_write_cstr
#     mov r0, r2
#     bl  debug_write_cstr
#     ldr r0, =msg_nl
#     bl  debug_write_cstr
# 1:
# .endif

#     mov r0, r5
#     mov r1, r6
#     mov r2, r7
# .if CRT0_DEBUG
#     ldr r3, =msg_before_libc
#     mov r0, r3
#     bl  debug_write_cstr
# .endif

#     mov r0, r5
#     mov r1, r6
#     mov r2, r7
#     bl __libc_init_array

# .if CRT0_DEBUG
#     ldr r0, =msg_after_libc
#     bl  debug_write_cstr
# .endif

#     mov r0, r5
#     mov r1, r6
#     mov r2, r7
#     bl main

#     bl exit
# 1:
#     b 1b
.global _start
_start:

    /* argc */
    ldr r0, [sp]

    /* argv */
    add r1, sp, #4

    /*
     * envp = &argv[argc + 1]
     *
     * argv 后面有一个 NULL
     */
    add r2, r1, r0, lsl #2
    add r2, r2, #4

    /*
     * environ = envp
     */
    ldr r3, =environ
    str r2, [r3]

    /*
     * 保存 argc argv envp
     */
    mov r5, r0
    mov r6, r1
    mov r7, r2

    /*
     * C++ constructors
     */
    mov r0, r5
    mov r1, r6
    mov r2, r7

    bl __libc_init_array

    /*
     * 调 main(argc, argv, envp)
     */
    mov r0, r5
    mov r1, r6
    mov r2, r7

    bl main

    /*
     * exit(main_ret)
     *
     * main 返回值已经在 r0
     */
    bl exit

1:
    b 1b

.if CRT0_DEBUG
.section .rodata
hex_digits:
    .ascii "0123456789abcdef"
msg_crt0_enter:
    .asciz "crt0: sp="
msg_argc:
    .asciz "crt0: argc="
msg_argv:
    .asciz "crt0: argv="
msg_envp:
    .asciz "crt0: envp="
msg_argv0:
    .asciz "crt0: argv0="
msg_before_libc:
    .asciz "crt0: before __libc_init_array\n"
msg_after_libc:
    .asciz "crt0: after __libc_init_array\n"
msg_nl:
    .asciz "\n"

.section .bss
.align 4
hex_buf:
    .space 12
.endif
