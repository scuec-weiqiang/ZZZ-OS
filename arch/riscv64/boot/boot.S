.section .text.entry
.globl  _start
_start:
    csrw mie, zero  # 禁用所有中断
    csrw satp, zero

    // 将所有异常委托给S模式处理
    li t0, 0xffff
    csrw medeleg, t0
    li t0, 0xffff
    csrw mideleg, t0

    // 开启sstc拓展
    csrr t0, mie
    li t1, 1 << 5
    or t0, t0, t1
    csrr t0,menvcfg
    li t1, 1 << 63
    or t0, t0, t1
    csrw menvcfg, t0
    // 允许s模式使用stimecmp
    csrr t0, mcounteren
    li t1, 2
    or t0, t0, t1
    csrw mcounteren, t0

    // 整个用户空间设置保护
    li t0, 0x3fffffffffffff
    csrw pmpaddr0, t0
    li t0, 0xf
    csrw pmpcfg0, t0

    csrr t0, mstatus
    li t1, ~(3 << 11)      # 清零MPP位的掩码
    and t0, t0, t1         # 先清零
    li t1, 1 << 11
    or t0, t0, t1 
    csrw mstatus, t0
    la t0, smode
    csrw mepc, t0 
    mret

smode:
    csrw sie, zero  # 禁用所有中断
    csrw satp, zero
    sfence.vma x0, x0

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

    la t0, _early_stack_end
    li t1, 0xffffffff40000000
    add t0, t0, t1
    mv sp, t0  # 设置早期栈指针
    csrw sscratch, sp  # sscratch存放内核栈指针

    li a0, 0        /* hartid */
    li a1, 0xffffffffc0400000        /* dtb phys addr */

    la t0, init_kernel
    li t1, 0xffffffff40000000
    add t0, t0, t1  # 转换为虚拟地址
    jr t0


    # li a0, 0x10000000
    # // 输出 "START" 表示内核已启动
    # li t6, 'S'
    # sb t6, 0(a0)
    # li t6, 'T'
    # sb t6, 0(a0)
    # li t6, 'A'
    # sb t6, 0(a0)
    # li t6, 'R'
    # sb t6, 0(a0)
    # li t6, 'T'
    # sb t6, 0(a0)
    # li t6, '\n'
    # sb t6, 0(a0)


#     csrw mie, zero  # 禁用所有中断
#     csrw satp, zero  # 禁用分页

#     // 读取hartid
#     csrr t0, mhartid
#     li t1, 0
#     beq t0, zero, _start_hart0  # 转换为物理地址
# _wait_hart:
#     wfi

# .global kernel_trap_entry
# _start_hart0:
#     la t0, kernel_trap_entry
#     csrw mtvec , t0  # 设置机器模式异常向量基址

#     // 将所有异常委托给S模式处理
#     li t0, 0xffff
#     csrw medeleg, t0
#     li t0, 0xffff
#     csrw mideleg, t0

#     // 开启sstc拓展
#     csrr t0, mie
#     li t1, 1 << 5
#     or t0, t0, t1
#     csrr t0,menvcfg
#     li t1, 1 << 63
#     or t0, t0, t1
#     csrw menvcfg, t0
#     // 允许s模式使用stimecmp
#     csrr t0, mcounteren
#     li t1, 2
#     or t0, t0, t1
#     csrw mcounteren, t0

#     // 整个用户空间设置保护
#     li t0, 0x3fffffffffffff
#     csrw pmpaddr0, t0
#     li t0, 0xf
#     csrw pmpcfg0, t0

#     csrr t0, mstatus
#     li t1, 1 << 11
#     or t0, t0, t1 
#     csrw mstatus, t0
#     la t0, _setup_paging
#     csrw mepc, t0  # 修正为使用转换后的物理地址
#     mret

# _setup_paging:
#     // 页表映射从内核开始的1G内存区域
#     li t0, 1<<0 | 1<<1 | 1<<2 | 1<<3  # PTE_V | PTE_R | PTE_W | PTE_X 

#     la t1, _early_pgtbl_start
    
#     // 映射高地址
#     li t2, 0xffffffffc0000000  # 内核虚拟地址起始
#     // 计算页表项索引
#     srli t3, t2, 30  # 虚拟地址右移30位取vpn[2]
#     andi t3, t3, 0x1ff  #掩码取10位得到纯净的vpn[2]
#     slli t3, t3, 3  # 每个页表项8字节
#     add t6, t1, t3 # 页表项地址

#     // 设置页表项，映射1G内存区域
#     li t4, 0x80000000  # 物理地址起始
#     srli t5, t4, 12  # 物理地址右移12位
#     slli t5, t5, 10  # 左移10位放入PTE的PPN位置
#     or t5, t5, t0  # 设置权限位
#     sd t5, 0(t6)  # 存储页表

#     // 恒等映射
#     li t2, 0x80000000
#     // 计算页表项索引
#     srli t3, t2, 30  # 虚拟地址右移30位取vpn[2]
#     andi t3, t3, 0x1ff  #掩码取10位得到纯净的vpn[2]
#     slli t3, t3, 3  # 每个页表项8字节
#     add t6, t1, t3 # 页表项地址

#     // 设置页表项，映射1G内存区域
#     li t4, 0x80000000  # 物理地址起始
#     srli t5, t4, 12  # 物理地址右移12位
#     slli t5, t5, 10  # 左移10位放入PTE的PPN位置
#     or t5, t5, t0  # 设置权限位
#     sd t5, 0(t6)  # 存储页表

#     // 将页表基地址写入satp寄存器
#     mv t0, t1
#     srli t0, t0, 12  # 右移12位
#     li t1, 8  # MODE=8 (SV39)
#     slli t1, t1, 60  # 左移60位放入MODE位置
#     or t0, t0, t1  # 组合satp值
#     csrw satp, t0
#     sfence.vma x0, x0  # 刷新地址转换缓存

#     la t0, _early_stack_end
#     li t1, 0xffffffff40000000
#     add t0, t0, t1
#     mv sp, t0  # 设置早期栈指针
#     csrw sscratch, sp  # sscratch存放内核栈指针

#     j init_kernel


    
.end