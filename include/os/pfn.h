#ifndef __KERNEL_PFN_H_
#define __KERNEL_PFN_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))   // 页掩码

#define SIZE_1M (1024 * 1024)
#define SIZE_2M (2 * SIZE_1M)
#define SIZE_1G (1024 * SIZE_1M)


#define phys_to_pfn(pa)  ((pa) >> PAGE_SHIFT)
#define pfn_to_phys(pfn) ((pfn) << PAGE_SHIFT)

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

#define PAGE_ALIGN(addr)  ALIGN_UP(addr, PAGE_SIZE)

#endif
