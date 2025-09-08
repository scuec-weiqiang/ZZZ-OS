/*******************************************************************************************
 * @FilePath: /ZZZ/arch/riscv64/riscv.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-30 13:45:35
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#ifndef RISCV_H
#define RISCV_H

#include "types.h"

typedef struct reg_context
{
    uintptr_t zero, ra, sp, gp, tp;
    uintptr_t t0, t1, t2, s0, s1;
    uintptr_t a0, a1, a2, a3, a4, a5, a6, a7;
    uintptr_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uintptr_t t3, t4, t5, t6;
    uintptr_t mepc ;
}reg_context_t;

#define MCAUSE_MASK_INTERRUPT 0x8000000000000000
#define MCAUSE_MASK_CAUSECODE 0x7fffffffffffffff

/*******************************************************************************************
 * @brief        : 
 * @return        {*}
*******************************************************************************************/
__SELF __INLINE uintptr_t mhartid_r()
{
    uintptr_t a;
    asm volatile("csrr %0,mhartid" : "=r"(a));
    return a;
}

__SELF __INLINE void tp_w(uintptr_t a)
{
    asm volatile("mv tp,%0"::"r"(a));
}

__SELF __INLINE uintptr_t tp_r()
{
    uintptr_t a;
    asm volatile("mv %0,tp" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mip_r()
{
    uintptr_t a;
    asm volatile("csrr %0,mip" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mip_w(uintptr_t a)
{
    asm volatile("csrw mip,%0"::"r"(a));
}



/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mie_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mie" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mie_w(uintptr_t a)
{   
    asm volatile("csrw mie,%0"::"r"(a));
}




/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mcause_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mcause" : "=r"(a));
    return a;
}



/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mepc_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mepc" : "=r"(a));
    return a;
}
__SELF __INLINE void mepc_w(uintptr_t a)
{   
    asm volatile("csrw mepc,%0"::"r"(a));
}




/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mstatus_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mstatus" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mstatus_w(uintptr_t a)
{   
    asm volatile("csrw mstatus,%0"::"r"(a));
}




/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE uintptr_t mscratch_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mscratch" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void mscratch_w(uintptr_t a)
{   
    asm volatile("csrw mscratch,%0"::"r"(a));
}




/*******************************************************************************************
 * @brief        : 
 * @return        {*}
*******************************************************************************************/
__SELF __INLINE uintptr_t mtvec_r()
{
    uintptr_t a;
    asm volatile("csrr %0,mtvec" : "=r"(a));
    return a;
}
/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
__SELF __INLINE void mtvec_w(uintptr_t a)
{   
    asm volatile("csrw mtvec,%0"::"r"(a));
}


__SELF __INLINE uintptr_t mtval_r()
{
    uintptr_t a;
    asm volatile("csrr %0,mtvec" : "=r"(a));
    return a;
}


__SELF __INLINE uintptr_t sp_r()
{   
    uintptr_t a;
    asm volatile("mv %0,sp" : "=r"(a));
    return a;
}

__SELF __INLINE void sp_w(uintptr_t a)
{   
    asm volatile("mv sp,%0"::"r"(a));
}

__SELF __INLINE void satp_w(uintptr_t a)
{   
    asm volatile("sfence.vma zero, zero");
    asm volatile("csrw satp,%0"::"r"(a));
    asm volatile("sfence.vma zero, zero");
}

__SELF __INLINE void medeleg_w(uintptr_t a)
{   
    asm volatile("csrw medeleg,%0"::"r"(a));
}
__SELF __INLINE uintptr_t medeleg_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,medeleg" : "=r"(a));
    return a;
}

__SELF __INLINE void mideleg_w(uintptr_t a)
{
    asm volatile("csrw mideleg,%0"::"r"(a));
}

__SELF __INLINE uintptr_t mideleg_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,mideleg" : "=r"(a));
    return a;
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software

__SELF __INLINE void sie_w(uintptr_t a)
{
    asm volatile("csrw sie,%0"::"r"(a));
}


__SELF __INLINE uintptr_t sie_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,sie" : "=r"(a));
    return a;
}

__SELF __INLINE uintptr_t sip_r()
{
    uintptr_t a;
    asm volatile("csrr %0,sip" : "=r"(a));
    return a;
}
#define SIP_SSIP (1 << 1) // software
/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void sip_w(uintptr_t a)
{
    asm volatile("csrw sip,%0"::"r"(a));
}
/***************************************************************
 * @description: Read supervisor status register
 * @return {*} Current sstatus value
***************************************************************/
__SELF __INLINE uintptr_t sstatus_r()
{
    uintptr_t a;
    asm volatile("csrr %0,sstatus" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: Write supervisor status register
 * @param {uintptr_t} a [in]: Value to write
 * @return {*}
***************************************************************/
__SELF __INLINE void sstatus_w(uintptr_t a)
{
    asm volatile("csrw sstatus,%0"::"r"(a));
}
__SELF __INLINE uintptr_t sscratch_r()
{   
    uintptr_t a;
    asm volatile("csrr %0,sscratch" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: 
 * @param {uintptr_t} a [in/out]:  
 * @return {*}
***************************************************************/
__SELF __INLINE void sscratch_w(uintptr_t a)
{   
    asm volatile("csrw sscratch,%0"::"r"(a));
}

/***************************************************************
 * @description: Read supervisor trap handler base address
 * @return {*} Current stvec value
***************************************************************/
__SELF __INLINE uintptr_t stvec_r()
{
    uintptr_t a;
    asm volatile("csrr %0,stvec" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: Write supervisor trap handler base address
 * @param {uintptr_t} a [in]: Value to write
 * @return {*}
***************************************************************/
__SELF __INLINE void stvec_w(uintptr_t a)
{
    asm volatile("csrw stvec,%0"::"r"(a));
}

/***************************************************************
 * @description: Read supervisor exception program counter
 * @return {*} Current sepc value
***************************************************************/
__SELF __INLINE uintptr_t sepc_r()
{
    uintptr_t a;
    asm volatile("csrr %0,sepc" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: Write supervisor exception program counter
 * @param {uintptr_t} a [in]: Value to write
 * @return {*}
***************************************************************/
__SELF __INLINE void sepc_w(uintptr_t a)
{
    asm volatile("csrw sepc,%0"::"r"(a));
}

/***************************************************************
 * @description: Read supervisor trap cause
 * @return {*} Current scause value
***************************************************************/
__SELF __INLINE uintptr_t scause_r()
{
    uintptr_t a;
    asm volatile("csrr %0,scause" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: Read supervisor trap value
 * @return {*} Current stval value
***************************************************************/
__SELF __INLINE uintptr_t stval_r()
{
    uintptr_t a;
    asm volatile("csrr %0,stval" : "=r"(a));
    return a;
}

/***************************************************************
 * @description: Read supervisor address translation register
 * @return {*} Current satp value
***************************************************************/
__SELF __INLINE uintptr_t satp_r()
{
    uintptr_t a;
    asm volatile("csrr %0,satp" : "=r"(a));
    return a;
}

__SELF __INLINE void pmpcfg0_w(uintptr_t a)
{
    asm volatile("csrw pmpcfg0,%0"::"r"(a));
}


__SELF __INLINE void pmpaddr0_w(uintptr_t a)
{
    asm volatile("csrw pmpaddr0,%0"::"r"(a));
}

__SELF __INLINE void sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

// #define M_TO_U(x)    __PROTECT( 
//     mstatus_w(mscratch_r() & ~(3<<11));  
//     mepc_w((uintptr_t)(x)); 
//     asm volatile ("mv a0, %0": : "r"(mhartid_r())); 
//     asm volatile("mret"); 
// )

__SELF __INLINE uintptr_t get_hart_id_s()
{
    uintptr_t a;
    asm volatile("mv %0,tp" : "=r"(a));
    return a;
}

#define M_TO_S(x)    __PROTECT( \
    mstatus_w(mstatus_r() & ~(3<<11));\
    mstatus_w(mstatus_r() | (1<<11));  \
    mepc_w((uintptr_t)(x)); \
    asm volatile("mret"); \
)

// #define M_TO_M(x)    __PROTECT( 
//     mstatus_w(mscratch_r() | (3<<11));  
//     mepc_w((uintptr_t)(x)); 
//     asm volatile ("mv a0, %0": : "r"(mhartid_r())); 
//     asm volatile("mret"); 
// )

#endif