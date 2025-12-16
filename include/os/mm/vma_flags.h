#ifndef __KERNEL_VMA_FLAGS_H
#define __KERNEL_VMA_FLAGS_H

typedef enum vma_flags{
    VMA_R       = 1 << 0,    // 可读
    VMA_W       = 1 << 1,    // 可写
    VMA_X       = 1 << 2,    // 可执行

    VMA_USER    = 1 << 3,    // 用户可访问
    VMA_KERNEL  = 1 << 4,    // 内核可访问

    VMA_DIRTY   = 1 << 5,    // 脏页
} vma_flags_t;

#endif