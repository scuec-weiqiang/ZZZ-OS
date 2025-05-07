# arch/riscv64/qemu_virt/trap_macros.s（宏定义）
#ifndef TRAP_MACROS_S
#define TRAP_MACROS_S

# 宏：保存通用寄存器到栈（参数：寄存器列表、起始偏移）
.macro SAVE_REGS reg_list, offset
    .set __offset, \offset
    .foreach reg, \reg_list {
        sd \reg, __offset(sp)
        .set __offset, __offset + 8
    }
.endm

# 宏：恢复通用寄存器从栈（参数：寄存器列表、起始偏移）
.macro RESTORE_REGS reg_list, offset
    .set __offset, \offset
    .foreach reg, \reg_list {
        ld \reg, __offset(sp)
        .set __offset, __offset + 8
    }
.endm

.macro SAVE_CTX
    csrrw sp, sscratch, sp

    .set reg_list, x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28,x29,x30,x31
    SAVE_REGS reg_list, 8

    csrr t0, sepc
    sd t0, 264(sp)  // sepc（偏移=32*8 + 8=264，32个通用寄存器占256字节，+8是x1的偏移）
    csrr t0, sstatus
    sd t0, 272(sp)  // sstatus（+8）
    csrr t0, scause
    sd t0, 280(sp)  // scause（+8）
    csrr t0, stval
    sd t0, 288(sp)  // stval（+8）
.endm

.macro RESTORE_CTX
    ld t0, 288(sp)
    csrw stval, t0
    ld t0, 280(sp)
    csrw scause, t0
    ld t0, 272(sp)
    csrw sstatus, t0
    ld t0, 256(sp)
    csrw sepc, t0
    RESTORE_REGS reg_list, 8 
.endm

#endif