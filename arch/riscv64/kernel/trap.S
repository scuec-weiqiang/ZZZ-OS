.set PT_RA,      0
.set PT_SP,      8
.set PT_GP,      16
.set PT_TP,      24
.set PT_T0,      32
.set PT_T1,      40
.set PT_T2,      48
.set PT_S0,      56
.set PT_S1,      64
.set PT_A0,      72
.set PT_A1,      80
.set PT_A2,      88
.set PT_A3,      96
.set PT_A4,      104
.set PT_A5,      112
.set PT_A6,      120
.set PT_A7,      128
.set PT_S2,      136
.set PT_S3,      144
.set PT_S4,      152
.set PT_S5,      160
.set PT_S6,      168
.set PT_S7,      176
.set PT_S8,      184
.set PT_S9,      192
.set PT_S10,     200
.set PT_S11,     208
.set PT_T3,      216
.set PT_T4,      224
.set PT_T5,      232
.set PT_T6,      240
.set PT_SEPC,    248
.set PT_SSTATUS, 256
.set PT_SCAUSE,  264
.set PT_STVAL,   272
.set PT_SIZE,    280
.set THREAD_SIZE, 8192
.set TI_CPU,      124
.set SSTATUS_SPP, 0x100

.section .text.trap, "ax"
.align 2

.global kernel_trap_entry
kernel_trap_entry:
    csrrw sp, sscratch, sp
    beqz sp, trap_from_kernel

trap_from_user:
    addi sp, sp, -PT_SIZE

    sd ra, PT_RA(sp)
    sd gp, PT_GP(sp)
    sd tp, PT_TP(sp)
    sd t0, PT_T0(sp)
    sd t1, PT_T1(sp)
    sd t2, PT_T2(sp)
    sd s0, PT_S0(sp)
    sd s1, PT_S1(sp)
    sd a0, PT_A0(sp)
    sd a1, PT_A1(sp)
    sd a2, PT_A2(sp)
    sd a3, PT_A3(sp)
    sd a4, PT_A4(sp)
    sd a5, PT_A5(sp)
    sd a6, PT_A6(sp)
    sd a7, PT_A7(sp)
    sd s2, PT_S2(sp)
    sd s3, PT_S3(sp)
    sd s4, PT_S4(sp)
    sd s5, PT_S5(sp)
    sd s6, PT_S6(sp)
    sd s7, PT_S7(sp)
    sd s8, PT_S8(sp)
    sd s9, PT_S9(sp)
    sd s10, PT_S10(sp)
    sd s11, PT_S11(sp)
    sd t3, PT_T3(sp)
    sd t4, PT_T4(sp)
    sd t5, PT_T5(sp)
    sd t6, PT_T6(sp)

    csrr t0, sscratch
    sd t0, PT_SP(sp)

    csrw sscratch, zero
    j trap_save_csr


trap_from_kernel:
    csrrw sp, sscratch, sp

    addi sp, sp, -PT_SIZE

    sd ra, PT_RA(sp)
    sd gp, PT_GP(sp)
    sd tp, PT_TP(sp)
    sd t0, PT_T0(sp)
    sd t1, PT_T1(sp)
    sd t2, PT_T2(sp)
    sd s0, PT_S0(sp)
    sd s1, PT_S1(sp)
    sd a0, PT_A0(sp)
    sd a1, PT_A1(sp)
    sd a2, PT_A2(sp)
    sd a3, PT_A3(sp)
    sd a4, PT_A4(sp)
    sd a5, PT_A5(sp)
    sd a6, PT_A6(sp)
    sd a7, PT_A7(sp)
    sd s2, PT_S2(sp)
    sd s3, PT_S3(sp)
    sd s4, PT_S4(sp)
    sd s5, PT_S5(sp)
    sd s6, PT_S6(sp)
    sd s7, PT_S7(sp)
    sd s8, PT_S8(sp)
    sd s9, PT_S9(sp)
    sd s10, PT_S10(sp)
    sd s11, PT_S11(sp)
    sd t3, PT_T3(sp)
    sd t4, PT_T4(sp)
    sd t5, PT_T5(sp)
    sd t6, PT_T6(sp)

    addi t0, sp, PT_SIZE
    sd t0, PT_SP(sp)

    j trap_save_csr


trap_save_csr:
    li t0, -THREAD_SIZE
    and t0, sp, t0
    # t0 现在存储了当前线程thread_info的地址,同时也是pt_regs的地址，因为 pt_regs在thread_info的开头
    # 内核态时，tp存了当前cpu_id;用户态tp则有其他用处，这里恢复tp为当前cpu_id，方便后续使用
    lw tp, TI_CPU(t0) 

    # 下面存储sepc, sstatus, scause, stval寄存器的值到pt_regs中
    csrr t0, sepc
    sd t0, PT_SEPC(sp)

    csrr t0, sstatus
    sd t0, PT_SSTATUS(sp)

    csrr t0, scause
    sd t0, PT_SCAUSE(sp)

    csrr t0, stval
    sd t0, PT_STVAL(sp)

    mv a0, sp
    call trap_handler

    sd a0, PT_SEPC(sp)

    # 依据当前的sstatus寄存器的SPP位判断是返回用户态还是内核态
    # spp表示进入trap前的特权级别，SPP位为1表示内核态，SPP位为0表示用户态
    # 如果SPP位为1，说明当前是内核态，返回内核态；如果SPP位为0，说明当前是用户态，返回用户态
    ld t0, PT_SSTATUS(sp)
    andi t0, t0, SSTATUS_SPP
    bnez t0, return_to_kernel

    # 返回用户态还需要处理一些额外的事情，比如执行调度，还有类似中断下半部处理
return_to_user:
    mv a0, sp
    call trap_prepare_user_return
    mv sp, a0

    ld t0, PT_SEPC(sp)
    csrw sepc, t0

    ld t0, PT_SSTATUS(sp)
    csrw sstatus, t0

    li t0, -THREAD_SIZE
    and t0, sp, t0
    li t1, THREAD_SIZE
    add t0, t0, t1
    csrw sscratch, t0

    j restore_regs

    # 恢复到o内核态则比较简单，直接恢复寄存器即可
return_to_kernel:
    ld t0, PT_SEPC(sp)
    csrw sepc, t0

    ld t0, PT_SSTATUS(sp)
    csrw sstatus, t0

    csrw sscratch, zero

    j restore_regs


restore_regs:
    ld ra, PT_RA(sp)
    ld gp, PT_GP(sp)
    ld tp, PT_TP(sp)

    ld t2, PT_T2(sp)

    ld s0, PT_S0(sp)
    ld s1, PT_S1(sp)

    ld a0, PT_A0(sp)
    ld a1, PT_A1(sp)
    ld a2, PT_A2(sp)
    ld a3, PT_A3(sp)
    ld a4, PT_A4(sp)
    ld a5, PT_A5(sp)
    ld a6, PT_A6(sp)
    ld a7, PT_A7(sp)

    ld s2, PT_S2(sp)
    ld s3, PT_S3(sp)
    ld s4, PT_S4(sp)
    ld s5, PT_S5(sp)
    ld s6, PT_S6(sp)
    ld s7, PT_S7(sp)
    ld s8, PT_S8(sp)
    ld s9, PT_S9(sp)
    ld s10, PT_S10(sp)
    ld s11, PT_S11(sp)

    ld t3, PT_T3(sp)
    ld t4, PT_T4(sp)
    ld t5, PT_T5(sp)
    ld t6, PT_T6(sp)

    ld t1, PT_T1(sp)
    ld t0, PT_T0(sp)

    ld sp, PT_SP(sp)

    sret