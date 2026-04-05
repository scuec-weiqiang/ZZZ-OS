.section .text.entry
.globl  _start
_start:
    // 关闭中断
    cpsid if

    // 关闭MMU和cache
    mrc p15,0,r0,c1,c0,0
    bic r0,r0,#1          @ SCTLR.M
    bic r0,r0,#(1 << 2)   @ SCTLR.C
    bic r0,r0,#(1 << 12)  @ SCTLR.I
    mcr p15,0,r0,c1,c0,0
    isb

    @ 将vector base 设为0xffff0000
    ldr r1, =(1<<13)
    mrc p15,0,r0,c1,c0,0
    orr r0,r0,r1
    mcr p15,0,r0,c1,c0,0
    isb

    ldr r0, =_early_pgtbl_start
    sub r0, r0, #0x40000000   @ r1为页表基地址

    mov r2, r0
    mov r3, #4096

clear:
    mov r7,#0
    str r7,[r2],#4
    subs r3,r3,#1
    bne clear

    mov r1, #0x02
    orr r1, r1, #0xe00 @ 页表项属性：可读写、可执行

    add r2, r0, #(0x800*4) @ r2为开始写入页表的位置
    mov r3, #0x80000000 @ r3为开始映射的虚拟地址,这里映射 pa = 0x80000000 -> va = 0x80000000，len = 512M
    mov r4, #0x80000000 @ r4为开始映射的物理地址

    mov r7, #512 @ r7用来计数

loop0:
    cmp r7, #0
    beq done0

    ldr r6, =0xFFF00000
    and r5, r4, r6  @ 保留物理地址的高20位
    orr r5, r5, r1 @ 加上属性位

    str r5, [r2], #4 @ 将页表项写入内存

    @ add r2, r2, #4 @ 移动到下一个页表项位置
    add r4, r4, #0x100000 @ 下一个1M内存区域

    sub r7, r7, #1 @ 计数器减1
    b loop0

done0:

    add r2, r0, #(0xc00 * 4) @ r2为开始写入页表的位置
    mov r3, #0xc0000000 @ r3为开始映射的虚拟地址,这里映射 pa = 0x80000000 -> va = 0xc0000000，len = 512M
    mov r4, #0x80000000 @ r4为开始映射的物理地址
    mov r7, #512 @ r4用来计数

loop1:
    cmp r7, #0
    beq done1

    ldr r6, =0xFFF00000
    and r5, r4, r6 @ 保留物理地址的高20位
    orr r5, r5, r1 @ 加上属性位

    str r5, [r2], #4 @ 将页表项写入内存

    @ add r2, r2, #4 @ 移动到下一个页表项位置
    add r4, r4, #0x100000 @ 映射下一个1M内存区域

    sub r7, r7, #1 @ 计数器减1
    b loop1

done1:
    
    // 将页表基地址写入TTBR0寄存器
    mcr p15, 0, r0, c2, c0, 0

    // 设置域访问控制寄存器，允许访问所有域
    mov r1, #0xFFFFFFFF
    mcr p15, 0, r1, c3, c0, 0

    @ invalidate TLB
    mov r1,#0
    mcr p15,0,r1,c8,c7,0
    
    dsb                         @ 确保TLB无效化完成
    isb                         @ 确保后续指令使用新TLB

    @ 启用MMU
    mrc p15,0,r1,c1,c0,0
    orr r1,r1,#1
    orr r1,r1,#(1 << 2)   @ 启用数据cache
    orr r1,r1,#(1 << 12)  @ 启用指令cache
    mcr p15,0,r1,c1,c0,0

    isb 

    ldr r0, =_early_stack_end

    @ 设置不同模式下的栈
    @ SVC  
    cps #0x13
    sub r0, r0, #0x2000
    mov sp, r0

    @ IRQ
    cps #0x12
    sub r0, r0, #0x1000
    mov sp, r0

    @ FIQ
    cps #0x11
    sub r0, r0, #0x1000
    mov sp, r0

    @ Abort
    cps #0x17
    sub r0, r0, #0x1000
    mov sp, r0

    @ Undef 
    cps #0x1b
    sub r0, r0, #0x1000
    mov sp, r0

    @ 回到 SVC 
    cps #0x13

    ldr r0, =0
    ldr r1, =0x80000000

    cpsie i
    
    b init_kernel

