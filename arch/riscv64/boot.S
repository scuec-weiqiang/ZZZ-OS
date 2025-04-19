#include "save_reg.S"
.equ STACK_SIZE,4096 #定义一个处理器的栈的大小
.section .text.init
.global _start
_start:
    csrw mie, zero # 首先关闭所有中断

    csrr tp, mhartid # 读取hartid暂存到tp中
    # bne tp, zero, stack_init # 0核去清bss段，其他核心直接去设置自己的栈
    bne tp, zero, wait # 0核去清bss段，其他核心休眠

bss_init:
    # 核心0初始化bss段,一次写入4个字节
    la a0, _bss_start
    la a1, _bss_end
bss_clear_loop:
    sd zero, 0(a0)# 向内存中写8个字节 0
    addi a0, a0 , 8 #指向下一块地址
    bltu a0, a1, bss_clear_loop #  

stack_init:
    li t0 , STACK_SIZE
    mul t1 , s0 , t0 # t0 = hartid * STACK_SIZE 

# _stack_start/end定义在链接文件中
#             (1)                              (2)                                  (3)
# |---.....----|-----------核心1的栈-------------|-------------核心0的栈--------------|
#              ^ \                                                                  /^
#              |   ----------------------------------------------------------------  |
#              |                                |                                    | 
#         _stack_start                          v                               _stack_end 
#                                       STACK_SIZE * 核心数 
    la a0, _stack_end
    sub sp,a0,t1 # 栈底 = _stack_end - hartid * STACK_SIZE
 # 栈设置完成

    # 给内核分配一个上下文，在没有任务执行时，内核用这个上下文运行
    la a0,_kernel_reg_ctx_start
    csrw mscratch, a0

    mv a0, tp # 将hartid作为c语言的main函数参数
    j main

wait:
    wfi
    j wait

# ################### 异常与中断向量表 #########################
.section .text.trap
.balign 4
.global trap_entry
trap_entry:
    _store

    csrr a0, mepc
    csrr a1, mcause
    call trap_handler

    csrw mepc, a0
    _load
    mret
.end

