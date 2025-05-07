#include "trap_macros.S"  // 包含宏定义

.section .text.trap
.balign 4
.global trap_entry
trap_entry:
    SAVE_CTX    

    csrr a0, mepc
    csrr a1, mcause
    csrr a2, mscratch
    call trap_handler
    csrw mepc, a0

    RESTORE_CTX

    # # #切换到内核栈
    # # li t0 , CONTEXT_SIZE
    # # mul t1 , tp , t0 # t1 = hartid * CONTEXT_SIZE 
    # # la a0, _kernel_reg_ctx_start
    # # add a1,a0,t1 
    # # LOAD sp, 2*REG_SIZE(a1) 
    # li t0 , STACK_SIZE
    # mul t1 , tp , t0 # t1 = hartid * STACK_SIZE 
    # la t2, _kernel_stack_end
    # sub sp,t2,t1 # 栈底 = _stack_end - hartid * STACK_SIZE

    # csrr a0, mepc
    # csrr a1, mcause
    # csrr a2, mscratch
    # call trap_handler
    # csrw mepc, a0

    # _load
    # mret
.end