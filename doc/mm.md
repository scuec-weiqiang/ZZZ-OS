# 内存管理子系统
**内存管理子系统 采用分层设计，将架构相关细节、页表通用逻辑与映射策略解耦，以支持多架构、多级页表以及灵活的映射策略拓展。**

[TOC]

# 1 总体架构
## 1.1 页表与内存映射
**[架构相关页表层](#21架构相关页表层)**
主要内容为**架构强相关的基础操作函数**，包括多级页表特性描述，PTE编码/解码，页表索引计算，TLB刷新等基础函数。
该层仅提供最小硬件抽象，不涉及页表遍历策略、映射粒度选择或内存管理策略。

**[通用页表逻辑层](#22通用页表层)**
它对页表逻辑进行了统一抽象，对上层**提供页表操作的统一接口**，如遍历，查找，映射，支持多级页表与不同页大小。
该层刻意保持“原子化”设计，**不尝试提供覆盖所有场景的统一映射函数**，而是为上层策略提供可组合的基础能力。

**[映射管理层](#23映射管理层)**
引入 mm_struct 与 VMA 管理机制，实现 VMA 的插入、删除、自动拆分与合并。根据虚拟地址、物理地址对齐与映射长度，自动选择合适页大小进行映射，最终调用通用页表层完成具体映射。。

## 1.2 内存分配
**物理内存管理**
从设备树读取物理内存信息；[memblock](../../mm/memblock.c) 根据设备树信息，对内存区域进行管理、裁剪、合并、reserved 处理，得到空闲内存区域，实现前期内存分配；[physmem](../../mm/physmem.c) 对memblock整理的空闲内存块 **构建 mem_map**（struct page 数组）用于描述每个物理页框，是**后续所有页级内存管理的基础**。

**[Buddy System](../../mm/buddy.c)（页级分配器）**
以 physmem 中物理页框为基本单位，按阶(order)管理空闲内存块，order = N表示块大小为2^N^个物理页；支持指定阶的内存块分配、释放，并自动完成块的拆分与合并。

**[Slab分配器](../../mm/slab.c)（小对象分配器）**
通过 Buddy System 获得一个page，在page上进行小对象分配；预定义 15 种固定大小的内存块规格（8~2048 字节），为不同大小的小内存申请提供适配；支持自定义内存块规格。

# 2. 页表与内存映射具体实现
## 2.1 架构相关页表层
具体实现在 arch/xx/mm/ 目录下，这里以riscv64架构为例。

文件 [arch/riscv64/mm/pgtbl.c][riscv64_pgtbl.c] 中有 `struct pgtable_features` 的具体实现，此结构体定义在[pgtbl_types.h](../include/os/mm/pgtbl_types.h)中，它负责描述页表特性，这是支持不同架构、不同级数页表的核心。**通过将页表结构信息数据化，使通用页表层无需感知具体架构细节**。

例如 `struct pgtable_features riscv_features` 描述了riscv架构Sv39页表的特性。
```c
/* 页表层级描述 */
struct pgtable_level {
    int level;              // 层级号（从0开始）
    int bits;               // 该层级占用的虚拟地址位数
    size_t page_size;       // 该层级对应的页面大小
    const char *name;       // 层级名称
};

static const struct pgtable_features riscv_features = {
    .va_bits = 39,          // 虚拟地址为39位
    .pa_bits = 56,          // 物理地址为56位
    .support_levels = 3,    // 这是一个三级页表
    .page_shift = 12,       // 基础页大小的偏移为12位

    // 此页表支持的特性
    .features = PGTABLE_FEATURE_HUGE_PAGES |
                PGTABLE_FEATURE_GLOBAL_PAGES |
                PGTABLE_FEATURE_USER_PAGES |
                PGTABLE_FEATURE_ACCESSED_DIRTY,
                
    // 描述了每级页表的大小、该层级在一个虚拟地址中占用的位数等信息
    .level = (struct pgtable_level[]) {
        {0, 9, PAGE_SIZE * 512 * 512, "PGD"},  // 1GB
        {1, 9, PAGE_SIZE * 512, "PMD"},        // 2MB  
        {2, 9, PAGE_SIZE, "PTE"},              // 4KB
    },
};
```
`arch_pgtbl_init()` 会将此结构体绑定到具体的页表结构体中。

具体函数功能通过函数名就能了解。这些函数都比较简单，是操作页表的最基本函数。函数实现和具体架构相关，移植到其他平台时只需实现相关函数即可。它们声明在 [arch/xx/include/asm/pgtbl.h](../arch/riscv64/include/asm/pgtbl.h) 中。
## 2.2 通用页表层
通用页表层对页表进行了统一抽象。在实际映射中，映射地址大小不尽相同，要支持不同架构多级页表，设计一个包揽一切的映射函数是不切实际的。

该层函数设计较为克制，每个函数通常只负责一项明确的页表操作，例如在某一级写入页表叶、将页表叶拆分为子页表，或将满足条件的子页表合并为页表叶。

下面是最主要的几个函数
```c 
int pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va, int target_level, pgtbl_walk_cb cb, void *arg)
void pgtbl_split(pgtable_t *pgtbl, pte_t *pte, int level) 
void pgtbl_merge(pgtable_t *pgtbl, pte_t *table, pte_t *table_pte)
```
`pgtbl_walk`函数负责循环到目标层，在目标层执行指定的callback函数；将它与具体的callback函数结合，即可实现映射、解除映射、查找等功能。

`pgtbl_split`函数将一个页表叶拆分成一整个下级页表。例如将一片地址区域原本按2MB大小映射，但后面需要解除8k大小映射，就需要将这片区域的页表叶拆成512个4k大小的页表叶。

`pgtbl_merge`函数与`pgtbl_split`函数含义相反，用于合并页表。

## 2.3 映射管理层
### 2.3.1 VMA设计与不重叠约束
仅仅使用页表对某个区域进行映射是不够的，在映射完毕后，操作系统或程序并不知道映射了哪些区域，因此需要VMA用于**记录与管理映射的虚拟地址**。

这里VMA实现较为简单，本质是一些包含地址信息的结构体所串成的链表。链表每个节点代表一个地址区域，链表中的区域不会有重叠。

当添加新映射时，如果不同节点组成连续地址，且权限标志位相同时，VMA会自动将几个节点合并成一个节点；同样地，当需要解除映射时，VMA也会自动拆分节点。但无论怎么拆分与合并，都会**严格保证节点的地址区域不会重叠**。

当上层函数需要映射某个地址时，实际上是将需要映射的地址信息添加到对应的VMA中，一般情况下**不会直接进行映射**。当要对映射的区域进行操作时，会触发缺页异常，**在异常处理函数里才会对虚拟地址区间分配物理内存，并进行实际映射操作**，也就是***lazy map***策略；如果直接执行映射操作，就是***eager map***。

### 2.3.2 lazy map 与 eager map 的边界划分
**lazy map并不是在任何情况下都适用。**

例如：
内核地址空间中的映射通常具有以下特点：
- 映射对象确定（内核代码、数据、设备寄存器等）
- 映射失败不可恢复
- 不允许在关键路径中触发缺页异常

因此对于内核而言，会采用***eager map***策略。

同时，对于设备内存映射MMIO,有：
- 不对应可分配的普通物理内存
- 访问时序严格
- 通常禁止缓存、禁止换出

设备地址空间无法通过缺页异常“按需生成”，因此 MMIO 映射也必须是 eager 的。

此外，对于明确要求物理连续性的映射（如：do_map(mm, vaddr, paddr, ...)），通常意味着映射对象已经存在，且不可延迟生成，因此也采用eager map。

总之，lazy map 与 eager map 的划分遵循以下原则：
- 是否允许缺页中断
- 是否存在明确的物理对象
- 错误是否可回复

### 2.3.3 本系统中的实现划分
接口层面，通过参数区分映射策略：
```c
int do_mmap(struct mm_struct *mm,
           virt_addr_t vaddr,
           phys_addr_t paddr,
           size_t size,
           vma_flags_t flags,
           int lazy_or_eager);
```
`lazy_or_eager`用于指明是否立即写入页表
[riscv64_pgtbl.c]:../arch/riscv64/mm/pgtbl.c