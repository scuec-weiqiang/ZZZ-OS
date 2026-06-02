.syntax unified
.text
.align 4

.global __syscall0
__syscall0:
    push {r7}
    mov r7, r0
    mov r0, #0
    svc #0
    pop {r7}
    bx lr

.global __syscall1
__syscall1:
    push {r7}
    mov r7, r0
    mov r0, r1
    svc #0
    pop {r7}
    bx lr

.global __syscall2
__syscall2:
    push {r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    svc #0
    pop {r7}
    bx lr

.global __syscall3
__syscall3:
    push {r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    svc #0
    pop {r7}
    bx lr

.global __syscall4
__syscall4:
    push {r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    ldr r3, [sp, #4]
    svc #0
    pop {r7}
    bx lr

.global __syscall5
__syscall5:
    push {r4, r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    ldr r3, [sp, #8]
    ldr r4, [sp, #12]
    svc #0
    pop {r4, r7}
    bx lr

.global __syscall6
__syscall6:
    push {r4, r5, r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    ldr r3, [sp, #12]
    ldr r4, [sp, #16]
    ldr r5, [sp, #20]
    svc #0
    pop {r4, r5, r7}
    bx lr
