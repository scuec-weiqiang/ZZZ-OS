#ifndef RISCV_H
#define RISCV_H

#include <os/types.h>

#define MCAUSE_MASK_INTERRUPT 0x8000000000000000
#define MCAUSE_MASK_CAUSECODE 0x7fffffffffffffff

static inline reg_t mhartid_r()
{
    reg_t a;
    asm volatile("csrr %0,mhartid" : "=r"(a));
    return a;
}

static inline void tp_w(reg_t a)
{
    asm volatile("mv tp,%0" ::"r"(a));
}

static inline reg_t tp_r()
{
    reg_t a;
    asm volatile("mv %0,tp" : "=r"(a));
    return a;
}

static inline reg_t mip_r()
{
    reg_t a;
    asm volatile("csrr %0,mip" : "=r"(a));
    return a;
}

static inline void mip_w(reg_t a)
{
    asm volatile("csrw mip,%0" ::"r"(a));
}

static inline reg_t mie_r()
{
    reg_t a;
    asm volatile("csrr %0,mie" : "=r"(a));
    return a;
}

static inline void mie_w(reg_t a)
{
    asm volatile("csrw mie,%0" ::"r"(a));
}

static inline reg_t mcause_r()
{
    reg_t a;
    asm volatile("csrr %0,mcause" : "=r"(a));
    return a;
}

static inline reg_t mepc_r()
{
    reg_t a;
    asm volatile("csrr %0,mepc" : "=r"(a));
    return a;
}
static inline void mepc_w(reg_t a)
{
    asm volatile("csrw mepc,%0" ::"r"(a));
}

static inline reg_t mstatus_r()
{
    reg_t a;
    asm volatile("csrr %0,mstatus" : "=r"(a));
    return a;
}

static inline void mstatus_w(reg_t a)
{
    asm volatile("csrw mstatus,%0" ::"r"(a));
}

static inline reg_t mscratch_r()
{
    reg_t a;
    asm volatile("csrr %0,mscratch" : "=r"(a));
    return a;
}

static inline void mscratch_w(reg_t a)
{
    asm volatile("csrw mscratch,%0" ::"r"(a));
}

static inline reg_t mtvec_r()
{
    reg_t a;
    asm volatile("csrr %0,mtvec" : "=r"(a));
    return a;
}

static inline void mtvec_w(reg_t a)
{
    asm volatile("csrw mtvec,%0" ::"r"(a));
}

static inline reg_t mtval_r()
{
    reg_t a;
    asm volatile("csrr %0,mtvec" : "=r"(a));
    return a;
}

static inline reg_t sp_r()
{
    reg_t a;
    asm volatile("mv %0,sp" : "=r"(a));
    return a;
}

static inline void sp_w(reg_t a)
{
    asm volatile("mv sp,%0" ::"r"(a));
}

static inline void satp_w(reg_t a)
{
    asm volatile("sfence.vma zero, zero");
    asm volatile("csrw satp,%0" ::"r"(a));
    asm volatile("sfence.vma zero, zero");
}

static inline void medeleg_w(reg_t a)
{
    asm volatile("csrw medeleg,%0" ::"r"(a));
}
static inline reg_t medeleg_r()
{
    reg_t a;
    asm volatile("csrr %0,medeleg" : "=r"(a));
    return a;
}

static inline void mideleg_w(reg_t a)
{
    asm volatile("csrw mideleg,%0" ::"r"(a));
}

static inline reg_t mideleg_r()
{
    reg_t a;
    asm volatile("csrr %0,mideleg" : "=r"(a));
    return a;
}

static inline void menvcfg_w(reg_t a)
{
    asm volatile("csrw menvcfg,%0" ::"r"(a));
}

static inline reg_t menvcfg_r()
{
    reg_t a;
    asm volatile("csrr %0,menvcfg" : "=r"(a));
    return a;
}

static inline void mcounteren_w(reg_t a)
{
    asm volatile("csrw mcounteren,%0" ::"r"(a));
}

static inline reg_t mcounteren_r()
{
    reg_t a;
    asm volatile("csrr %0,mcounteren" : "=r"(a));
    return a;
}

static inline reg_t time_r()
{
    reg_t a;
    asm volatile("csrr %0,time" : "=r"(a));
    return a;
}

static inline void stimecmp_w(reg_t a)
{
    asm volatile("csrw stimecmp,%0" ::"r"(a));
}

static inline reg_t stimecmp_r()
{
    reg_t a;
    asm volatile("csrr %0,stimecmp" : "=r"(a));
    return a;
}

#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software

static inline void sie_w(reg_t a)
{
    asm volatile("csrw sie,%0" ::"r"(a));
}

static inline reg_t sie_r()
{
    reg_t a;
    asm volatile("csrr %0,sie" : "=r"(a));
    return a;
}

static inline reg_t sip_r()
{
    reg_t a;
    asm volatile("csrr %0,sip" : "=r"(a));
    return a;
}
#define SIP_SSIP (1 << 1) // software

static inline void sip_w(reg_t a)
{
    asm volatile("csrw sip,%0" ::"r"(a));
}

static inline reg_t sstatus_r()
{
    reg_t a;
    asm volatile("csrr %0,sstatus" : "=r"(a));
    return a;
}

static inline void sstatus_w(reg_t a)
{
    asm volatile("csrw sstatus,%0" ::"r"(a));
}
static inline reg_t sscratch_r()
{
    reg_t a;
    asm volatile("csrr %0,sscratch" : "=r"(a));
    return a;
}

static inline void sscratch_w(reg_t a)
{
    asm volatile("csrw sscratch,%0" ::"r"(a));
}

static inline reg_t stvec_r()
{
    reg_t a;
    asm volatile("csrr %0,stvec" : "=r"(a));
    return a;
}

static inline void stvec_w(reg_t a)
{
    asm volatile("csrw stvec,%0" ::"r"(a));
}

static inline reg_t sepc_r()
{
    reg_t a;
    asm volatile("csrr %0,sepc" : "=r"(a));
    return a;
}

static inline void sepc_w(reg_t a)
{
    asm volatile("csrw sepc,%0" ::"r"(a));
}

static inline reg_t scause_r()
{
    reg_t a;
    asm volatile("csrr %0,scause" : "=r"(a));
    return a;
}

static inline reg_t stval_r()
{
    reg_t a;
    asm volatile("csrr %0,stval" : "=r"(a));
    return a;
}

static inline reg_t satp_r()
{
    reg_t a;
    asm volatile("csrr %0,satp" : "=r"(a));
    return a;
}

static inline void pmpcfg0_w(reg_t a)
{
    asm volatile("csrw pmpcfg0,%0" ::"r"(a));
}

static inline void pmpaddr0_w(reg_t a)
{
    asm volatile("csrw pmpaddr0,%0" ::"r"(a));
}



// #define M_TO_U(x)    __PROTECT(
//     mstatus_w(mscratch_r() & ~(3<<11));
//     mepc_w((reg_t)(x));
//     asm volatile ("mv a0, %0": : "r"(mhartid_r()));
//     asm volatile("mret");
// )

static inline reg_t get_hart_id_s()
{
    reg_t a;
    asm volatile("mv %0,tp" : "=r"(a));
    return a;
}

#define M_TO_S(x) __PROTECT(             \
    mstatus_w(mstatus_r() & ~(3 << 11)); \
    mstatus_w(mstatus_r() | (1 << 11));  \
    mepc_w((reg_t)(x));              \
    asm volatile("mret");)

#define S_TO_U(x) __PROTECT(            \
    sstatus_w(sstatus_r() & ~(1 << 8)); \
    sepc_w((reg_t)(x));             \
    asm volatile("sret");)
// #define M_TO_M(x)    __PROTECT(
//     mstatus_w(mscratch_r() | (3<<11));
//     mepc_w((reg_t)(x));
//     asm volatile ("mv a0, %0": : "r"(mhartid_r()));
//     asm volatile("mret");
// )

#endif