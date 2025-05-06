#include "reg_save.S"

.section .text.trap
.balign 4
.global trap_entry
trap_entry:
    _store

    #切换到内核栈
    li t0 , CONTEXT_SIZE
    mul t1 , tp , t0 # t1 = hartid * CONTEXT_SIZE 
    la a0, _kernel_reg_ctx_start
    add a1,a0,t1 
    LOAD sp, 2*REG_SIZE(a1) 

    csrr a0, mepc
    csrr a1, mcause
    csrr a2, mscratch
    call trap_handler
    csrw mepc, a0

    _load
    mret
.end