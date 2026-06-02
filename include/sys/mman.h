#ifndef ZZZ_OS_SYS_MMAN_H
#define ZZZ_OS_SYS_MMAN_H

#include <stddef.h>
#include <sys/types.h>
#include <uapi/mman_defs.h>

#define MAP_FAILED ((void *)-1)

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t length, int prot);

#endif
