#include <asm/uaccess.h>
#include <os/pfn.h>

int copy_from_user(char *dst, char* src, size_t len)
{
    size_t copied = 0;

    while (copied < len) {
        char *srcp = src + copied;
        char *dstp = dst + copied;
        size_t page_left = PAGE_SIZE - ((uintptr_t)srcp & (PAGE_SIZE - 1));
        size_t n = page_left;

        if (n > len - copied)
            n = len - copied;

        __copy_from_user(dstp, srcp, n);

        copied += n;
    }

    return copied;
}

int copy_to_user(char *dst, char* src, size_t len) {
    size_t copied = 0;

    while (copied < len) {
        char *srcp = src + copied;
        char *dstp = dst + copied;
        size_t page_left = PAGE_SIZE - ((uintptr_t)dstp & (PAGE_SIZE - 1));
        size_t n = page_left;

        if (n > len - copied)
            n = len - copied;

        __copy_to_user(dstp, srcp, n);

        copied += n;
    }

    return len - copied;
}
