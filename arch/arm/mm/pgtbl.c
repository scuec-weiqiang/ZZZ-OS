
#include <os/types.h>
#include <os/mm/pgtbl_types.h>
#include <os/mm/pgprot.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/pfn.h>
#include <os/check.h>
#include <os/kva.h>
#include <os/bitops.h>
#include <asm/pgtbl.h>

#define TYPE_TABLE                   (0x1)
#define TYPE_SECTION                 (0x2)

#define TYPE_PAGE                    (0x1)
#define TYPE_LARGE_PAGE              (0x0)

#define LEVEL0_TYPE_MASK            GENMASK(1,0)
#define LEVEL0_SECTION_B_MASK       BIT(2) // B位为1表示内存可缓冲
#define LEVEL0_SECTION_C_MASK       BIT(3) // C位为1表示内存可缓存
#define LEVEL0_SECTION_XN_MASK      BIT(4) // XN位为1表示禁止执行
#define LEVEL0_SECTION_DOMAIN_MASK  GENMASK(8,5) // 域为0表示访问权限由AP位控制
#define LEVEL0_SECTION_P_MASK       BIT(9) // 令人疑惑的位，在ARMv7-A中，P位在第9位, P位为1表示段被预取, 但在某些文档中也被描述为保留位，必须为0。这里我们暂时将其定义为预取位，但实际使用时需要根据具体的ARMv7实现来确定。
#define LEVEL0_SECTION_AP_MASK      GENMASK(11, 10)  // AP位为00表示无访问权限, 01表示只读, 10表示读写, 11表示读写
#define LEVEL0_SECTION_TEX_MASK     GENMASK(14, 12) // 和B、C位配合使用，用于扩展内存属性
#define LEVEL0_SECTION_APX_MASK     BIT(15) // 和AP配合使用，APX位为1表示更严格的访问权限
#define LEVEL0_SECTION_S_MASK       BIT(16) // S位在第16位, S位为1表示共享
#define LEVEL0_SECTION_NG_MASK      BIT(17) // nG位在第17位, nG位为1表示非全局
#define LEVEL0_SUPERSECTION_MASK    BIT(18) // 超级段位在第18位, 超级段为1表示16MB段
#define LEVEL0_SECTION_RESERVED2    BIT(19) // 保留位,必须为0  
#define LEVEL0_SECTION_PA_MASK      GENMASK(31, 20) // 物理地址
// #define LEVEL0_SUPERSECTION_PA_MASK GENMASK(31, 24) // 超级段的物理地址

#define LEVEL0_TABLE_SBZ_MASK       GENMASK(4, 2) // 表项中第2-4位必须为0
#define LEVEL0_TABLE_DOMAIN_MASK    GENMASK(8,5) // 域为0表示访问权限由AP位控制
#define LEVEL0_TABLE_P_MASK         BIT(9) 
#define LEVEL0_TABLE_PA_MASK        GENMASK(31, 10)  // 二级页表的物理地址在第10-31位


#define LEVEL1_PAGE_XN_MASK      BIT(0) // XN位为1表示禁止执行
#define LEVEL1_TYPE_MASK         BIT(1) // 为1表明使用小页
#define LEVEL1_PAGE_B_MASK       BIT(2) // B位为1表示内存可缓冲
#define LEVEL1_PAGE_C_MASK       BIT(3) // C位为1表示内存可缓存
#define LEVEL1_PAGE_AP_MASK      GENMASK(5, 4)  // AP位为00表示无访问权限, 01表示只读, 10表示读写, 11表示读写
#define LEVEL1_PAGE_TEX_MASK     GENMASK(8, 6) // 和B、C位配合使用，用于扩展内存属性
#define LEVEL1_PAGE_APX_MASK     BIT(9) // 和AP配合使用，APX位为1表示更严格的访问权限
#define LEVEL1_PAGE_S_MASK       BIT(10) // S位在第16位, S位为1表示共享
#define LEVEL1_PAGE_NG_MASK      BIT(11) // nG位在第17位, nG位为1表示非全局
#define LEVEL1_PAGE_PA_MASK      GENMASK(31, 12) // 物理地址

// #define LEVEL1_LARGE_PAGE_MASK    BIT(0) // 为1表示LARGE PAGE
// #define LEVEL1_LARGE_PA_MASK GENMASK(31, 24) // 超级段的物理地址

/* armv7页表特性 */
static const struct pgtable_features armv7_features = {
    .va_bits = 32,          
    .pa_bits = 32,
    .support_levels = 2,
    .page_shift = 12,

    .features = PGTABLE_FEATURE_HUGE_PAGES | 
                PGTABLE_FEATURE_GLOBAL_PAGES |
                PGTABLE_FEATURE_USER_PAGES,

    .level = (struct pgtable_level[]) {
        {0, 12, SIZE_1M,SIZE_16K, "PGD"},  // 1M
        {1, 8, SIZE_4K, SIZE_1K,"PTE"},              // 4KB
    },
};

static inline void set_level0_type(pte_t *entry, pgdesc_type_t type) {
    switch (type) {
        case PGTBL_DESC_TABLE:
            SET_VAL(entry->val, LEVEL0_TYPE_MASK, TYPE_TABLE); // 设置类型为表项
            return;
        case PGTBL_DESC_PAGE:
            SET_VAL(entry->val, LEVEL0_TYPE_MASK, TYPE_SECTION); // 设置类型为段
            return;
        default:
            return; // 无效类型
    }
}
static inline void set_level0_pa(pgdesc_type_t type, pte_t *entry, phys_addr_t pa) {
    switch (type) {
        case PGTBL_DESC_TABLE:
            SET_VAL(entry->val, LEVEL0_TABLE_PA_MASK, GET_VAL(pa, LEVEL0_TABLE_PA_MASK));
            return;
        case PGTBL_DESC_PAGE:
            // printk("set_level0_pa: type=PGTBL_DESC_PAGE, pa=%xu, GET_VAL = %x\n", pa, GET_VAL(pa, LEVEL0_SECTION_PA_MASK));
            SET_VAL(entry->val, LEVEL0_SECTION_PA_MASK, GET_VAL(pa, LEVEL0_SECTION_PA_MASK));
            return;
        default:
            return; // 无效类型
    }
}
static phys_addr_t get_level0_pa(pte_t *entry) {
    pteval_t val = entry->val;
    phys_addr_t pa = 0;
    int type = GET_VAL(val, LEVEL0_TYPE_MASK);
    switch (type) {
        case TYPE_TABLE:
            SET_VAL(pa, LEVEL0_TABLE_PA_MASK, GET_VAL(val, LEVEL0_TABLE_PA_MASK));
            break;
        case TYPE_SECTION:
            SET_VAL(pa, LEVEL0_SECTION_PA_MASK, GET_VAL(val, LEVEL0_SECTION_PA_MASK));
            break;
        default:
            break; // 无效类型
    }
    return pa;
}
static void set_level0_flags(pte_t *entry, pgprot_t flags) {
    uint32_t val = entry->val;

    uint32_t ap = 0;
    uint32_t apx = 0;

    /* ---------- access permission ---------- */
    if (flags & PROT_USER) {
        if (flags & PROT_WRITE) {
            /* user rw */
            ap  = 0b11;
            apx = 0;
        } else {
            /* user ro */
            ap  = 0b10;
            apx = 1;
        }
    } else { /* kernel only */
        if (flags & PROT_WRITE) {
            /* privileged rw */
            ap  = 0b01;
            apx = 0;
        } else {
            /* privileged ro */
            ap  = 0b01;
            apx = 1;
        }
    }

    SET_VAL(val, LEVEL0_SECTION_AP_MASK, ap);
    SET_VAL(val, LEVEL0_SECTION_APX_MASK, apx);

    /* ---------- execute permission ---------- */
    if (flags & PROT_EXEC){
        SET_VAL(val, LEVEL0_SECTION_XN_MASK, 0);
    } else {
        SET_VAL(val, LEVEL0_SECTION_XN_MASK, 1);
    }
       

    /* ---------- memory type ---------- */
    if (flags & PROT_DEVICE) {
        /* device memory */
        SET_VAL(val, LEVEL0_SECTION_TEX_MASK, 0);
        SET_VAL(val, LEVEL0_SECTION_C_MASK, 0);
        SET_VAL(val, LEVEL0_SECTION_B_MASK, 1);
        SET_VAL(val, LEVEL0_SECTION_S_MASK, 1);
    } else if (flags & PROT_UNCACHED) {
        /* strongly ordered memory */
        SET_VAL(val, LEVEL0_SECTION_TEX_MASK, 1);
        SET_VAL(val, LEVEL0_SECTION_C_MASK, 0);
        SET_VAL(val, LEVEL0_SECTION_B_MASK, 0);
        SET_VAL(val, LEVEL0_SECTION_S_MASK, 1);
    } else {
        /* normal memory */
        SET_VAL(val, LEVEL0_SECTION_TEX_MASK, 1);
        SET_VAL(val, LEVEL0_SECTION_C_MASK, 1);
        SET_VAL(val, LEVEL0_SECTION_B_MASK, 1);
        SET_VAL(val, LEVEL0_SECTION_S_MASK, 1);
    }

    /* ---------- global ---------- */
    if (flags & PROT_GLOBAL) {
        SET_VAL(val, LEVEL0_SECTION_NG_MASK, 0);
    }

    /* ---------- domain ---------- */
    SET_VAL(val, LEVEL0_SECTION_DOMAIN_MASK, 0);

    entry->val = val;
}
static pgprot_t get_level0_flags(pte_t *entry) {
    pteval_t val = entry->val;
    pgprot_t flags = PROT_NONE;
    uint32_t ap = 0;
    uint32_t apx = 0;

    ap = GET_VAL(val, LEVEL0_SECTION_AP_MASK);
    apx = GET_VAL(val,LEVEL0_SECTION_APX_MASK);

    // printk("get_level0_flags: ap=%bu, apx=%bu\n", ap, apx);

    if (apx == 0) {
        switch (ap) {
            case 0b00:
                SET_VAL(flags, PROT_READ, 0);
                SET_VAL(flags, PROT_WRITE, 0);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b01:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b11:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 1);
                break;
            default:
                break;
        }
    } else {
        switch (ap) {
            case 0b01:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 0);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b10:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 0);
                break;
            default:
                break;
        }
    }

    if (GET_VAL(val, LEVEL0_SECTION_XN_MASK)) {
        SET_VAL(flags, PROT_EXEC, 0);
    } else {
        SET_VAL(flags, PROT_EXEC, 1);
    }

    if (GET_VAL(val, LEVEL0_SECTION_C_MASK)) {
        if (GET_VAL(val, LEVEL0_SECTION_B_MASK)) {
            /* normal memory */
            SET_VAL(flags, PROT_DEVICE, 0);
        } else {
            /* strongly ordered memory */
            SET_VAL(flags, PROT_DEVICE, 1);
        }
    } else {
        /* device memory */
        SET_VAL(flags, PROT_DEVICE, 1);
    }

    if (GET_VAL(val, LEVEL0_SECTION_NG_MASK)) {
        SET_VAL(flags, PROT_GLOBAL, 0);
    } else {
        SET_VAL(flags, PROT_GLOBAL, 1);
    }

    return flags;
}


static inline void set_level1_type(pte_t *entry, pgdesc_type_t type) {
     switch (type) {
        case PGTBL_DESC_PAGE:
            SET_VAL(entry->val, LEVEL1_TYPE_MASK, TYPE_PAGE);
            return;
        default:
            return; // 无效类型
    }
}
static inline void set_level1_pa(pgdesc_type_t type, pte_t *entry, phys_addr_t pa) {
    switch (type) {
        // case PGTBL_DESC_HUGE:
        //     SET_VAL(entry->val, LEVEL1_HUGE_PA_MASK, GET_VAL(pa, LEVEL0_HUGE_PA_MASK));
        //     return;
        case PGTBL_DESC_PAGE:
            SET_VAL(entry->val, LEVEL1_PAGE_PA_MASK, GET_VAL(pa, LEVEL1_PAGE_PA_MASK));
            return;
        default:
            return; // 无效类型
    }
}
static phys_addr_t get_level1_pa(pte_t *entry) {
    pteval_t val = entry->val;
    phys_addr_t pa = 0;
    int type = GET_VAL(val, LEVEL1_TYPE_MASK);
    switch (type) {
        // case TYPE_LARGE_PAGE:
        //     SET_VAL(pa, LEVEL1_LARGE_PAGE_PA_MASK, GET_VAL(val, LEVEL1_TABLE_PA_MASK));
        //     break;
        case TYPE_PAGE:
            SET_VAL(pa, LEVEL1_PAGE_PA_MASK, GET_VAL(val, LEVEL1_PAGE_PA_MASK));
            break;
        default:
            break; // 无效类型
    }
    // printk("get_level1_pa: type=%d, entry val=%xu, get pa = %xu\n", type, entry->val, pa);
    return pa;
}
static void set_level1_flags(pte_t *entry, pgprot_t flags) {
     uint32_t val = entry->val;

    uint32_t ap = 0;
    uint32_t apx = 0;

    /* ---------- access permission ---------- */

    if (flags & PROT_USER) {
        if (flags & PROT_WRITE) {
            /* user rw */
            ap  = 0b11;
            apx = 0;
        } else {
            /* user ro */
            ap  = 0b10;
            apx = 1;
        }
    } else { /* kernel only */
        if (flags & PROT_WRITE) {
            /* privileged rw */
            ap  = 0b01;
            apx = 0;
        } else {
            /* privileged ro */
            ap  = 0b01;
            apx = 1;
        }
    }

    SET_VAL(val, LEVEL1_PAGE_AP_MASK, ap);
    SET_VAL(val, LEVEL1_PAGE_APX_MASK, apx);

    /* ---------- execute permission ---------- */
    if (flags & PROT_EXEC){
        SET_VAL(val, LEVEL1_PAGE_XN_MASK, 0);
    } else {
        SET_VAL(val, LEVEL1_PAGE_XN_MASK, 1);
    }
       

    /* ---------- memory type ---------- */
    if (flags & PROT_DEVICE) {
        /* device memory */
        SET_VAL(val, LEVEL1_PAGE_TEX_MASK, 0);
        SET_VAL(val, LEVEL1_PAGE_C_MASK, 0);
        SET_VAL(val, LEVEL1_PAGE_B_MASK, 1);
        SET_VAL(val, LEVEL1_PAGE_S_MASK, 1);
    } else if (flags & PROT_UNCACHED) {
        /* strongly ordered memory */
        SET_VAL(val, LEVEL1_PAGE_TEX_MASK, 1);
        SET_VAL(val, LEVEL1_PAGE_C_MASK, 0);
        SET_VAL(val, LEVEL1_PAGE_B_MASK, 0);
        SET_VAL(val, LEVEL1_PAGE_S_MASK, 1);
    } else {
        /* normal memory */
        SET_VAL(val, LEVEL1_PAGE_TEX_MASK, 1);
        SET_VAL(val, LEVEL1_PAGE_C_MASK, 1);
        SET_VAL(val, LEVEL1_PAGE_B_MASK, 1);
        SET_VAL(val, LEVEL1_PAGE_S_MASK, 1);
    }

    /* ---------- global ---------- */
    if (flags & PROT_GLOBAL) {
        SET_VAL(val, LEVEL1_PAGE_NG_MASK, 0);
    }

    entry->val = val;
}
static pgprot_t get_level1_flags(pte_t *entry) {
    pteval_t val = entry->val;
    pgprot_t flags = PROT_NONE;
    uint32_t ap = 0;
    uint32_t apx = 0;

    ap = GET_VAL(val, LEVEL1_PAGE_AP_MASK);
    apx = GET_VAL(val,LEVEL1_PAGE_APX_MASK);

        // printk("get_level1_flags: ap=%bu, apx=%bu\n", ap, apx);

    if (apx == 0) {
        switch (ap) {
            case 0b00:
                SET_VAL(flags, PROT_READ, 0);
                SET_VAL(flags, PROT_WRITE, 0);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b01:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b11:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 1);
                break;
            default:
                break;
        }
    } else {
        switch (ap) {
            case 0b01:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 0);
                SET_VAL(flags, PROT_USER, 0);
                break;
            case 0b10:
                SET_VAL(flags, PROT_READ, 1);
                SET_VAL(flags, PROT_WRITE, 1);
                SET_VAL(flags, PROT_USER, 0);
                break;
            default:
                break;
        }
    }

    if (GET_VAL(val, LEVEL1_PAGE_XN_MASK)) {
        SET_VAL(flags, PROT_EXEC, 0);
    } else {
        SET_VAL(flags, PROT_EXEC, 1);
    }

    if (GET_VAL(val, LEVEL1_PAGE_C_MASK)) {
        if (GET_VAL(val, LEVEL1_PAGE_B_MASK)) {
            /* normal memory */
            SET_VAL(flags, PROT_DEVICE, 0);
        } else {
            /* strongly ordered memory */
            SET_VAL(flags, PROT_DEVICE, 1);
        }
    } else {
        /* device memory */
        SET_VAL(flags, PROT_DEVICE, 1);
    }

    if (GET_VAL(val, LEVEL1_PAGE_NG_MASK)) {
        SET_VAL(flags, PROT_GLOBAL, 0);
    } else {
        SET_VAL(flags, PROT_GLOBAL, 1);
    }

    return flags;
}

static inline void dcache_clean_mva_poc(uintptr_t addr)
{
    asm volatile(
        "mcr p15, 0, %0, c7, c10, 1\n"
        :
        : "r"(addr)
        : "memory"
    );
}

static inline void dcache_clean_mva_poc_range(uintptr_t start, uintptr_t end)
{
    for (uintptr_t addr = start; addr < end; addr += 32) {
        dcache_clean_mva_poc(addr);
    }
    asm volatile("dsb" : : : "memory");

}

void arch_pgtbl_entry_set_pa(pgtable_t *tbl, uint32_t level, pgdesc_type_t type, pte_t *entry, phys_addr_t pa) {
    switch (level) {
        case 0:
            set_level0_pa(type, entry, pa);return;
        case 1:
            set_level1_pa(type, entry, pa);return;
        default:
            return; // 无效级别
    }
}

phys_addr_t arch_pgtbl_entry_get_pa(pgtable_t *tbl, uint32_t level, pgdesc_type_t type, pte_t *entry) {
    switch (level) {
        case 0:
            // printk("arch_pgtbl_entry_get_pa: level=0, entry val=%xu\n", entry->val);
            return get_level0_pa(entry);
        case 1:
            // printk("arch_pgtbl_entry_get_pa: level=1, entry val=%xu\n", entry->val);
            return get_level1_pa(entry);
        default:
            return 0; // 无效级别
    }
}

uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va) {
    switch (level) {
        case 0:
            return GET_VAL(va, GENMASK(31, 20)); 
        case 1:
            return GET_VAL(va, GENMASK(19, 12)); // 二级页表索引在虚拟地址的12-31位
        default:
            return 0; // 无效级别
    }

}

void arch_pgtbl_entry_set_flags(pgtable_t *tbl, int level, pte_t* entry, pgprot_t flags) {
    switch (level) {
        case 0:
            set_level0_flags(entry, flags);
            break;
        case 1:
            set_level1_flags(entry, flags);
            break;
        default:
            // 无效级别
            break;
    }
}

pgprot_t arch_pgtbl_entry_get_flags(pgtable_t *tbl, int level, pte_t *entry) {
    switch (level) {
        case 0:
            return get_level0_flags(entry);
        case 1:
            // printk("arch_pgtbl_entry_get_flags: level=1, entry val=%xu\n", entry->val);
            return get_level1_flags(entry);
        default:
            return PROT_NONE; // 无效级别
    }
}

void arch_pgtbl_set_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry, phys_addr_t pa, pgprot_t flags) {
    switch (level) {
        case 0: set_level0_type(entry, type); break;
        case 1: set_level1_type(entry, type); break;
        default: break;
    } 
    
    arch_pgtbl_entry_set_pa(tbl, level, type, entry, pa);
        
    if (type == PGTBL_DESC_PAGE) {
        arch_pgtbl_entry_set_flags(tbl, level, entry, flags);
    }
    dcache_clean_mva_poc((uintptr_t)entry); // 确保页表项的修改被写回内存
}

void arch_pgtbl_clear_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry) {
    entry->val = 0; // 清除整个页表项
}

bool arch_pgtbl_entry_is_valid(pte_t *entry) {
    return GET_VAL(entry->val, LEVEL0_TYPE_MASK) != 0; // 只要最低两位不为0，就认为是有效项
}

bool arch_pgtbl_entry_is_leaf(pte_t *entry) {
    // 由于不支持large_page, 所有bit 1 为1的项都是叶子节点
    if (GET_VAL(entry->val, BIT(1)) == 1) {
        return true; // 一级页表中的段项是叶子节点
    } else {
        return false;
    }
}

void arch_pgtbl_sync_range(void *addr, size_t size) {
    uintptr_t start = ALIGN_DOWN((uintptr_t)addr, 32);
    uintptr_t end = ALIGN_UP((uintptr_t)addr + size, 32);
    dcache_clean_mva_poc_range(start, end);
}

void arch_pgtbl_flush() {
    asm volatile(
        "dsb\n"
        "mov r0, #0\n"
        "mcr p15, 0, r0, c8, c7, 0\n" 
        "dsb\n"
        "isb\n"
        :
        :
        : "r0", "memory"
    ); 
}

void arch_pgtbl_switch_to(pgtable_t *pgtbl) {
    asm volatile(
        /*
         * Force the short-descriptor walk to use TTBR0 for the full VA space.
         * Some boot environments may leave TTBCR configured with a split,
         * which would make high VAs (for example 0xf0000000) bypass TTBR0.
         */
        "mov r1, #0\n"
        "mcr p15, 0, r1, c2, c0, 2\n"
        "mcr p15, 0, %0, c2, c0, 0\n"
        "mcr p15, 0, r1, c8, c7, 0\n"
        "dsb\n"
        "isb\n"
        :
        : "r"(pgtbl->root_pa)
        : "r1", "memory"
    ); 
}

void arch_pgtbl_init(pgtable_t *tbl) {
    tbl->features = &armv7_features;
}

void arch_pgtbl_deinit(pgtable_t *tbl) {
    tbl->features = NULL;
    if (tbl->root) {
        kfree(tbl->root);
        tbl->root = NULL;
    }
}



