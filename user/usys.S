#include <syscall_num.h>

.syntax unified
.text
.align 4

.macro DO_SYSCALL name, nr
.global \name
\name:
    mov r7, #\nr
    svc #0
    bx lr
.endm

DO_SYSCALL print,  SYSCALL_PRINT
DO_SYSCALL yield,  SYSCALL_YIELD
DO_SYSCALL open,   SYSCALL_OPEN
DO_SYSCALL close,  SYSCALL_CLOSE
DO_SYSCALL read,   SYSCALL_READ
DO_SYSCALL write,  SYSCALL_WRITE
DO_SYSCALL creat,  SYSCALL_CREAT
DO_SYSCALL mkdir,  SYSCALL_MKDIR
DO_SYSCALL getpid, SYSCALL_GETPID
DO_SYSCALL exit,   SYSCALL_EXIT
