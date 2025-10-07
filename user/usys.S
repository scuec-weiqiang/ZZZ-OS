/**
 * @FilePath: /ZZZ/kernel/usys.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-19 14:01:28
 * @LastEditTime: 2025-05-02 18:56:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "syscall_num.h"

.global print
print:
    li a7, SYSCALL_PRINT
    ecall
    ret
    
.global yield
yield:
    li a7, SYSCALL_YIELD
    ecall
    ret


.global open
open:
    li a7, SYSCALL_OPEN
    ecall
    ret

.global close
close:
    li a7, SYSCALL_CLOSE
    ecall   
    ret

.global read
read:
    li a7, SYSCALL_READ
    ecall
    ret

.global write
write:
    li a7, SYSCALL_WRITE
    ecall   
    ret

.global creat
creat:
    li a7, SYSCALL_CREAT
    ecall
    ret

.global mkdir
mkdir:
    li a7, SYSCALL_MKDIR
    ecall
    ret

