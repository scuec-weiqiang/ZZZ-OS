#ifndef ZZZ_OS_UAPI_MMAN_DEFS_H
#define ZZZ_OS_UAPI_MMAN_DEFS_H

#ifndef PROT_NONE
#define PROT_NONE  0x0
#endif
#ifndef PROT_READ
#define PROT_READ  0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_EXEC
#define PROT_EXEC  0x4
#endif

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS

#endif
