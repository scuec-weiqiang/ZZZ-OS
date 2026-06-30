#include <mm/pgtbl_types.h>
#include <os/kva.h>
#include <mm/symbols.h>
#include <mm/pgtbl.h>
#include <os/mm.h>
#include <os/printk.h>
#include <mm/memblock.h>
#include <os/pfn.h>

void arch_initial_mm_init() {
    extern char _early_pgtbl_start[];
    pgtable_t old_pgdir = {
        .root = (void *)(&_early_pgtbl_start),
        .root_pa = KERNEL_PA(&_early_pgtbl_start),
    };
    extern void arch_pgtbl_init(pgtable_t *tbl);
    arch_pgtbl_init(&old_pgdir);

    pgtbl_switch_to(&old_pgdir);
    
    init_mm.pgdir = new_pgtbl();
    if (!init_mm.pgdir) {
        panic("failed to create kernel pgtable");
    }
    struct memblock_region *region = NULL;
    list_for_each_entry(region, &memblock.memory.region_head.node, struct memblock_region, node) {
        map(init_mm.pgdir, KERNEL_VA(region->base), region->base, region->size, PAGE_KERNEL|PROT_EXEC);
    }
    // map(init_mm.pgdir, 0x02020000,0x02020000, PAGE_SIZE, PAGE_DEVICE);

    // remap(init_mm.pgdir, trap_start, trap_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, text_start, text_size, PAGE_KERNEL_EXEC);

    // remap(init_mm.pgdir, data_start, data_size, PAGE_KERNEL);
    // remap(init_mm.pgdir, rodata_start, rodata_size, PAGE_KERNEL_RO);
    // remap(init_mm.pgdir, bss_start, bss_size, PAGE_KERNEL);
    // remap(init_mm.pgdir, initcall_start, initcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, exitcall_start, exitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, irqinitcall_start, irqinitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, irqexitcall_start, irqexitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, early_stack_start, early_stack_size, PAGE_KERNEL);
    
    pgtbl_switch_to(init_mm.pgdir);
}