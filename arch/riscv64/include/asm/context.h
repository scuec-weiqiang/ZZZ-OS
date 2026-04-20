#ifndef __ASM_CONTEXT_H
#define __ASM_CONTEXT_H
#include <os/types.h>

struct pt_regs
{
    reg_t ra, sp, gp, tp;
    reg_t t0, t1, t2, s0, s1;
    reg_t a0, a1, a2, a3, a4, a5, a6, a7;
    reg_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    reg_t t3, t4, t5, t6;
    reg_t sepc, sstatus, scause, stval;
};

// 内核上下文，只需要保存 callee-saved 寄存器
struct context 
{
    reg_t ra, sp;
    reg_t s0, s1, s2, s3;
    reg_t s4, s5, s6, s7;
    reg_t s8, s9, s10,s11;
};

#endif
