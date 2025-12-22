#ifndef __KERNEL_PAGE_FAULT_H
#define __KERNEL_PAGE_FAULT_H

#include <os/mm/mm_types.h>

int do_page_fault(struct mm_struct *mm, virt_addr_t fault_addr, int fault_flags);

#endif // __KERNEL_PAGE_FAULT_H