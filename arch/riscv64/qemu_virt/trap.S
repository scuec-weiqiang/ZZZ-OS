#include "trap_macros.S"  // 包含宏定义

.section .text.trap
.balign 4
.global machine_timer_trap_entry
machine_timer_trap_entry:
    csrrw a3, mscratch, a3
    sd a0, 0(a3)
    sd a1, 8(a3)
    sd a2, 16(a3)

    ld a0, 24(a3)# timecmp(hartid)的地址
    ld a1, 0(a0)# timecmp(hartid)的值
    ld a2, 32(a3)# systimer_hz的值
    add a2, a2, a1# 装载定时器
    sd a2, 0(a0)

    #手动触发software interrupt
    li a1, 2
    csrw sip, a1

    ld a2, 16(a3)
    ld a1, 8(a3)
    ld a0, 0(a3)
    csrrw a3, mscratch, a3

    mret

.balign 4
.global kernel_trap_entry
kernel_trap_entry:
    SAVE_CTX    

    csrr a0, mepc
    csrr a1, mcause
    csrr a2, mscratch
    call trap_handler
    csrw mepc, a0

    RESTORE_CTX

    sret
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