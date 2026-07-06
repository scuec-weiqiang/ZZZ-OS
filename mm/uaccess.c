#include <asm/uaccess.h>
#include <os/pfn.h>
#include <os/errno.h>
#include <os/sched.h>

int copy_from_user(char *dst, const char* src, size_t len) {
    int ret = 0;
    size_t copied = 0;
    unsigned long t = enable_user_access();

    while (copied < len) {
        char *srcp = (char*)src + copied;
        char *dstp = dst + copied;
        size_t page_left = PAGE_SIZE - ((uintptr_t)srcp & (PAGE_SIZE - 1));
        size_t n = page_left;

        if (n > len - copied)
            n = len - copied;

        ret = __copy_from_user(dstp, srcp, n);
        if (ret < 0) {
            restore_user_access(t);
            return ret;
        }

        copied += n;
    }

    restore_user_access(t);
    return len - copied;
}

int copy_to_user(char *dst, char* src, size_t len) {
    int ret = 0;
    size_t copied = 0;
    unsigned long t = enable_user_access();

    while (copied < len) {
        char *srcp = src + copied;
        char *dstp = dst + copied;
        size_t page_left = PAGE_SIZE - ((uintptr_t)dstp & (PAGE_SIZE - 1));
        size_t n = page_left;

        if (n > len - copied)
            n = len - copied;

        ret = __copy_to_user(dstp, srcp, n);
        if (ret < 0) {
            restore_user_access(t);
            return ret;
        }

        copied += n;
    }

    restore_user_access(t);
    return len - copied;
}

int copy_user_string(char *dst, size_t dst_len, uintptr_t user_ptr) {
    size_t i;

    if (!dst || dst_len == 0 || user_ptr == 0 || current->mm == NULL)
        return -EINVAL;

    for (i = 0; i < dst_len; i++) {
        if (copy_from_user(&dst[i], (const char *)user_ptr + i, 1) < 0)
            return -EFAULT;
        if (dst[i] == '\0')
            return 0;
    }

    dst[dst_len - 1] = '\0';
    return -ENAMETOOLONG;
}