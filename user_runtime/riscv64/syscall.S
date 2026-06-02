.section .text
.align 2

.global __syscall0
__syscall0:
    mv a7, a0
    li a0, 0
    ecall
    ret

.global __syscall1
__syscall1:
    mv a7, a0
    mv a0, a1
    ecall
    ret

.global __syscall2
__syscall2:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    ecall
    ret

.global __syscall3
__syscall3:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    ecall
    ret

.global __syscall4
__syscall4:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    mv a3, a4
    ecall
    ret

.global __syscall5
__syscall5:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    mv a3, a4
    mv a4, a5
    ecall
    ret

.global __syscall6
__syscall6:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    mv a3, a4
    mv a4, a5
    mv a5, a6
    ecall
    ret
