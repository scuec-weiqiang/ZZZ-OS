#ifndef RISCV_H
#define RISCV_H

#include "types.h"

typedef struct reg_context
{
    reg_t zero, ra, sp, gp, tp;
    reg_t t0, t1, t2, s0, s1;
    reg_t t0, a1, a2, a3, a4, a5, a6, a7;
    reg_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s1;
    reg_t t3, t4, t5, t6;
    reg_t mepc ;
}reg_context_t;

__SELF __INLINE reg_t mhartid_r()
{
    reg_t a;
    asm volatile("csrr %0,mhartid" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mip_r()
{
    reg_t a;
    asm volatile("csrr %0,mip" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @param {reg_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mip_w(reg_t a)
{
    asm volatile("csrw mip,%0"::"r"(a));
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mie_r()
{   
    reg_t a;
    asm volatile("csrr %0,mie" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @param {reg_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mie_w(reg_t a)
{   
    asm volatile("csrw mie,%0"::"r"(a));
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mcause_r()
{   
    reg_t a;
    asm volatile("csrr %0,mcause" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mepc_r()
{   
    reg_t a;
    asm volatile("csrr %0,mepc" : "=r"(a));
    return a;
}

__SELF __INLINE void mepc_w(reg_t a)
{   
    asm volatile("csrw mepc,%0"::"r"(a));
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mstatus_r()
{   
    reg_t a;
    asm volatile("csrr %0,mstatus" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @param {reg_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mstatus_w(reg_t a)
{   
    asm volatile("csrw mstatus,%0"::"r"(a));
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE reg_t mscratch_r()
{   
    reg_t a;
    asm volatile("csrr %0,mscratch" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @param {reg_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mscratch_w(reg_t a)
{   
    asm volatile("csrw mscratch,%0"::"r"(a));
}

#endif