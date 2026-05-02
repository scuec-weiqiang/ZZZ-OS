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
