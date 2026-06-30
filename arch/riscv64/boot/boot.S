.section .text.entry
.globl  _start
_start:
    mv s0, a0
    mv s1, a1

    mv tp, s0               # tp = cpu ID

    # CPU 0 继续正常初始化 
    csrw sie, zero  # 禁用所有中断
    csrw satp, zero
    sfence.vma x0, x0

    li t0, (1 << 18) # SUM 位,允许 S 模式访问用户内存
    csrs sstatus, t0

    /* 设置 S-mode trap */
    la t0, kernel_trap_entry
    li t1, 0xffffffff40000000
    add t0, t0, t1  # 转换为虚拟地址
    csrw stvec, t0

    // 页表映射从内核开始的1G内存区域
    li t0, 1<<0 | 1<<1 | 1<<2 | 1<<3  # PTE_V | PTE_R | PTE_W | PTE_X 

    la t1, _early_pgtbl_start
    
    // 映射高地址
    li t2, 0xffffffffc0000000  # 内核虚拟地址起始
    // 计算页表项索引
    srli t3, t2, 30  # 虚拟地址右移30位取vpn[2]
    andi t3, t3, 0x1ff  #掩码取10位得到纯净的vpn[2]
    slli t3, t3, 3  # 每个页表项8字节
    add t6, t1, t3 # 页表项地址

    // 设置页表项，映射1G内存区域
    li t4, 0x80000000  # 物理地址起始
    srli t5, t4, 12  # 物理地址右移12位
    slli t5, t5, 10  # 左移10位放入PTE的PPN位置
    or t5, t5, t0  # 设置权限位
    sd t5, 0(t6)  # 存储页表

    // 恒等映射
    li t2, 0x80000000
    // 计算页表项索引
    srli t3, t2, 30  # 虚拟地址右移30位取vpn[2]
    andi t3, t3, 0x1ff  #掩码取10位得到纯净的vpn[2]
    slli t3, t3, 3  # 每个页表项8字节
    add t6, t1, t3 # 页表项地址

    // 设置页表项，映射1G内存区域
    li t4, 0x80000000  # 物理地址起始
    srli t5, t4, 12  # 物理地址右移12位
    slli t5, t5, 10  # 左移10位放入PTE的PPN位置
    or t5, t5, t0  # 设置权限位
    sd t5, 0(t6)  # 存储页表

    // 将页表基地址写入satp寄存器
    mv t0, t1
    srli t0, t0, 12  # 右移12位
    li t1, 8  # MODE=8 (SV39)
    slli t1, t1, 60  # 左移60位放入MODE位置
    or t0, t0, t1  # 组合satp值

    csrw satp, t0
    sfence.vma x0, x0  # 刷新地址转换缓存

    .global init_thread_union
    la t0, init_thread_union

    # la t0, _early_stack_end
    li t1, 0xffffffff40000000
    add t0, t0, t1
    li t1, 0x2000
    add t0, t0, t1
    mv sp, t0  # 设置早期栈指针
    csrw sscratch, zero

    la t0, start_kernel
    li t1, 0xffffffff40000000
    add t0, t0, t1  # 转换为虚拟地址

    mv a0, s0
    mv a1, s1
    jr t0

.globl secondary_start
secondary_start:
    mv s0, a0               # s0 = hart ID
    mv s1, a1               # s1 = opaque from SBI hart_start
    mv tp, s0               # early cpu ID = hart ID

    csrw sie, zero
    csrw satp, zero
    sfence.vma x0, x0

    mv t0, tp
    la t1, secondary_data
    li t2, 24
    mul t2, t0, t2
    add t1, t1, t2

secondary_wait_release:
    ld t2, 0(t1)
    beqz t2, secondary_wait_release
    fence iorw, iorw

    ld sp, 8(t1)
    ld t2, 16(t1)

    # Temporarily enter the same early kernel mapping as CPU0.
    la t3, _early_pgtbl_start
    srli t3, t3, 12
    li t4, 8
    slli t4, t4, 60
    or t3, t3, t4
    csrw satp, t3
    sfence.vma x0, x0

    li t3, (1 << 18)
    csrs sstatus, t3

    la t3, kernel_trap_entry
    li t4, 0xffffffff40000000
    add t3, t3, t4
    csrw stvec, t3
    csrw sscratch, zero

    mv a0, tp
    mv a1, s1
    jr t2

/*
 * 辅助核自旋区
 * 每个 non-0 cpu 在这里等待 CPU0 释放
 * secondary_data[cpuid] 结构 (C 中定义):
 *   offset 0: go (8B)
 *   offset 8: stack (8B)
 *   offset 16: entry (8B)
 *   每 CPU 24 字节
 */
secondary_spin:
    mv t0, tp               # t0 = cpu ID
    la t1, secondary_data   # t1 = secondary_data 基址
    li t2, 24
    mul t2, t0, t2          # t2 = cpuid * 24
    add t1, t1, t2          # t1 = &secondary_data[cpuid]

spin_loop:
    ld t2, 0(t1)            # 读取 go 标志
    beqz t2, spin_loop      # go == 0 → 等待
    fence iorw, iorw        # 内存屏障

    ld sp, 8(t1)            # sp = secondary_data[cpuid].stack
    ld t2, 16(t1)           # t2 = secondary_data[cpuid].entry
    mv a0, tp               # a0 = cpuid (cpu ID)
    mv a1, s1               # a1 = dtb (from _start)
    jr t2                   # 跳转到 secondary_entry

# /*
#  * print_hex64(a0 = value)
#  * 输出 16 个十六进制字符到 UART0，不带 0x 前缀。
#  */
# .globl print_hex64
# print_hex64:
#     li t2, 0x10000000
#     li t3, 16

# 1:
#     srli t0, a0, 60
#     andi t0, t0, 0xf
#     li t1, 10
#     blt t0, t1, 2f
#     addi t0, t0, 'a' - 10
#     j 3f

# 2:
#     addi t0, t0, '0'

# 3:
#     sb t0, 0(t2)
#     slli a0, a0, 4
#     addi t3, t3, -1
#     bnez t3, 1b
#     ret

# .end
