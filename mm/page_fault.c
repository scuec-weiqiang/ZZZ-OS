#include <os/mm/vma.h>
#include <os/mm/mm_types.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/pfn.h>
#include <os/kmalloc.h>
#include <os/kva.h>

int do_page_fault(struct mm_struct *mm, virt_addr_t fault_addr, int fault_flags) {
    if (!mm) {
        panic("Invalid mm_struct pointer!\n");
        return -1; // 无效的内存描述符
    }

    struct vma *vma = vma_find(mm, fault_addr);
    if (!vma) {
        panic("No valid VMA found for address: %xu\n", fault_addr);
        return -1; // 找不到对应的VMA
    }

    virt_addr_t va = (virt_addr_t)page_alloc(1);

    map(mm->pgdir, fault_addr, KERNEL_PA(va), PAGE_SIZE, vma->flags);
 
    // 根据VMA信息处理缺页异常
    printk("Page fault at address: %xu, flags: %d\n", fault_addr, fault_flags);
    return 0; // 返回0表示处理成功
}