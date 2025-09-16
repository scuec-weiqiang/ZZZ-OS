#     .section .bss
#     .align 12                 # 页表必须按 4KiB 对齐
#     .global early_pgdir
# early_pgdir:
#     .skip 4096                # root page table (1 page)

#     .section .text.init
#     .global setup_vm
# setup_vm:
#     # t0 = &early_pgdir
#     la      t0, early_pgdir

#     #-------------------------------------------
#     # Entry 1: identity map VA=0x80000000 -> PA=0x80000000 (1GiB)
#     #-------------------------------------------
#     li      t2, (0x80000000 >> 30) & 0x1ff   # VPN2 index
#     slli    t2, t2, 3
#     add     t3, t0, t2

#     # 构造 PTE: PPN + flags
#     li      t4, (0x80000000 >> 30)           # PPN[2]
#     slli    t4, t4, 28                       # 对齐到位 (Sv39 L1 entry)
#     li      t5, 0b1110                      # V|R|W|X|U (示例，可改)
#     or      t4, t4, t5

#     sd      t4, 0(t3)

#     #-------------------------------------------
#     # Entry 2: high half map VA=0xffffffffc0000000 -> PA=0x80000000
#     #-------------------------------------------
#     li      t6, ((0xffffffffc0000000 >> 30) & 0x1ff)  # 高半 VPN2
#     slli    t6, t6, 3
#     add     a0, t0, t6        # 用 a0 代替 t7

#     li      a1, (0x80000000 >> 30)
#     slli    a1, a1, 28
#     or      a1, a1, t5

#     sd      a1, 0(a0)


#     ret
