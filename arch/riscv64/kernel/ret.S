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
.set THREAD_SIZE, 8192
.set SSTATUS_SPP, 0x100

.text
.align 2

.global arch_user_enter
arch_user_enter:
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

.global ret_from_fork
ret_from_fork:
    call sched_tail
    beqz s1, 1f

    mv a0, s0
    li t0, 0x2
    csrs sstatus, t0
    jalr s1

    ld t0, PT_SSTATUS(sp)
    andi t0, t0, SSTATUS_SPP
    beqz t0, 1f

    mv a0, zero
    call do_exit
1:
    mv a0, sp
    tail arch_user_enter
