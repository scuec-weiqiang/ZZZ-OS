#define LOAD        ld
#define STORE       sd
#define REG_SIZE    8
#define TRAP_FRAME_SIZE  REG_SIZE*36

.section .text.trap
.balign 4
.global kernel_trap_entry
kernel_trap_entry:
    csrrw sp, sscratch, sp
    addi sp, sp, -TRAP_FRAME_SIZE
    STORE ra,  1*REG_SIZE(sp)  
    STORE sp,  2*REG_SIZE(sp)  
    STORE gp,  3*REG_SIZE(sp)  
    STORE tp,  4*REG_SIZE(sp)  
    STORE t0,  5*REG_SIZE(sp)  
    STORE t1,  6*REG_SIZE(sp)  
    STORE t2,  7*REG_SIZE(sp)  
    STORE s0,  8*REG_SIZE(sp)  
    STORE s1,  9*REG_SIZE(sp)  
    STORE a0, 10*REG_SIZE(sp)  
    STORE a1, 11*REG_SIZE(sp)  
    STORE a2, 12*REG_SIZE(sp)  
    STORE a3, 13*REG_SIZE(sp)  
    STORE a4, 14*REG_SIZE(sp)  
    STORE a5, 15*REG_SIZE(sp)  
    STORE a6, 16*REG_SIZE(sp)  
    STORE a7, 17*REG_SIZE(sp)  
    STORE s2, 18*REG_SIZE(sp)  
    STORE s3, 19*REG_SIZE(sp)  
    STORE s4, 20*REG_SIZE(sp)  
    STORE s5, 21*REG_SIZE(sp)  
    STORE s6, 22*REG_SIZE(sp)  
    STORE s7, 23*REG_SIZE(sp)  
    STORE s8, 24*REG_SIZE(sp)  
    STORE s9, 25*REG_SIZE(sp)  
    STORE s10,26*REG_SIZE(sp)  
    STORE s11,27*REG_SIZE(sp)  
    STORE t3, 28*REG_SIZE(sp)  
    STORE t4, 29*REG_SIZE(sp)  
    STORE t5, 30*REG_SIZE(sp)  
    STORE t6, 31*REG_SIZE(sp)  
    csrr t0, sepc              
    STORE t0, 33*REG_SIZE(sp)  
    csrr t0, sstatus           
    STORE t0, 34*REG_SIZE(sp)  
    csrr t0, scause            
    STORE t0, 35*REG_SIZE(sp)  
    csrr t0, stval             
    STORE t0, 36*REG_SIZE(sp)    
   
    csrr a0, sepc
    csrr a1, scause
    csrr a2, sscratch
    call trap_handler
    csrw sepc, a0

    LOAD t0, 36*REG_SIZE(sp)  
    csrw stval, t0            
    LOAD t0, 35*REG_SIZE(sp)  
    csrw scause, t0           
    LOAD t0, 34*REG_SIZE(sp)  
    csrw sstatus, t0          
    LOAD t0, 33*REG_SIZE(sp)  
    csrw sepc, t0             
    LOAD t6, 31*REG_SIZE(sp)  
    LOAD t5, 30*REG_SIZE(sp)  
    LOAD t4, 29*REG_SIZE(sp)  
    LOAD t3, 28*REG_SIZE(sp)  
    LOAD s11,27*REG_SIZE(sp)  
    LOAD s10,26*REG_SIZE(sp)  
    LOAD s9, 25*REG_SIZE(sp)  
    LOAD s8, 24*REG_SIZE(sp)  
    LOAD s7, 23*REG_SIZE(sp)  
    LOAD s6, 22*REG_SIZE(sp)  
    LOAD s5, 21*REG_SIZE(sp)  
    LOAD s4, 20*REG_SIZE(sp)  
    LOAD s3, 19*REG_SIZE(sp)  
    LOAD s2, 18*REG_SIZE(sp)  
    LOAD a7, 17*REG_SIZE(sp)  
    LOAD a6, 16*REG_SIZE(sp)  
    LOAD a5, 15*REG_SIZE(sp)  
    LOAD a4, 14*REG_SIZE(sp)  
    LOAD a3, 13*REG_SIZE(sp)  
    LOAD a2, 12*REG_SIZE(sp)  
    LOAD a1, 11*REG_SIZE(sp)  
    LOAD a0, 10*REG_SIZE(sp)  
    LOAD s1, 9*REG_SIZE(sp)   
    LOAD s0, 8*REG_SIZE(sp)   
    LOAD t2, 7*REG_SIZE(sp)   
    LOAD t1, 6*REG_SIZE(sp)   
    LOAD t0, 5*REG_SIZE(sp)   
    LOAD tp, 4*REG_SIZE(sp)   
    LOAD gp, 3*REG_SIZE(sp)   
    LOAD sp, 2*REG_SIZE(sp)   
    LOAD ra, 1*REG_SIZE(sp) 
    addi sp, sp, TRAP_FRAME_SIZE  
    csrrw sp, sscratch, sp

    sret
.end