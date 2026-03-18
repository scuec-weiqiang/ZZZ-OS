.section .text.entry
.global _start

_start:

    cpsid if                 @ 关中断

    /* 关闭MMU */
    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#1
    mcr p15,0,r0,c1,c0,0
    isb

    /* UART 地址 */
    ldr r10, =0x02020040

    /* 设置 early stack */
    ldr r2, =_early_stack_end
    sub r2, r2, #0x40000000
    mov sp, r2

    /* 页表基地址 */
    ldr r0, =_early_pgtbl_start
    sub r0, r0, #0x40000000

    /* 清空页表 (16KB) */
    mov r1,#0
    mov r2,r0
    mov r3,#4096

clear_pgtable:
    str r1,[r2],#4
    subs r3,r3,#1
    bne clear_pgtable

    /* section 属性 */
    mov r1,#0x2
    orr r1,r1,#0xE00

    /***************************
     identity map
     VA 0x80000000 -> PA 0x80000000
     size 512MB
    ****************************/

    add r2,r0,#(0x800*4)
    mov r3,#0x80000000
    mov r4,#0x80000000
    mov r7,#512

map_identity:

    ldr r12, =0xFFF00000
    and r5,r4,r12
    orr r5,r5,r1
    str r5,[r2],#4

    add r4,r4,#0x100000
    subs r7,r7,#1
    bne map_identity

    /***************************
     kernel high map
     VA 0xC0000000 -> PA 0x80000000
    ****************************/

    add r2,r0,#(0xC00*4)
    mov r4,#0x80000000
    mov r7,#512

map_kernel:

    ldr r12,=0xFFF00000
    and r5,r4,r12
    orr r5,r5,r1
    str r5,[r2],#4

    add r4,r4,#0x100000
    subs r7,r7,#1
    bne map_kernel

    /***************************
     UART map
    ****************************/

    add r2,r0,#(0x20*4)
    
    ldr r3,=0x02000000
    and r5,r3,r12
    orr r5,r5,r1
    str r5,[r2]

    /***************************
     TTBR0
    ****************************/

    mcr p15,0,r0,c2,c0,0

    /* DACR */
    mov r1,#0xFFFFFFFF
    mcr p15,0,r1,c3,c0,0

    /***************************
     invalidate TLB
    ****************************/

    mov r1,#0
    mcr p15,0,r1,c8,c7,0

    dsb
    isb

    /***************************
     enable MMU
    ****************************/

    mrc p15,0,r1,c1,c0,0
    orr r1,r1,#1
    mcr p15,0,r1,c1,c0,0

    isb

    /***************************
     测试 UART
    ****************************/

    ldr r11, ='A'
    str r11,[r10]

    /***************************
     进入 C
    ****************************/

    ldr r0, =0
    ldr r1, =0x80000000

    b boot_main