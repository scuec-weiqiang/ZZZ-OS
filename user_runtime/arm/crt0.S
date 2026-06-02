.syntax unified
.section .text.startup, "ax"
.align 4
.global _start
.type _start, %function

.extern main
.extern exit
.extern __libc_init_array
.extern environ

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
