#ifndef CLINT_H
#define CLINT_H

#include "types.h"

#define CLINT_BASE          0x02000000

#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP(hartid)      (CLINT_BASE + (0x4000)+ 8*(hartid))

#define CLINT_MSIP(hartid)          (CLINT_BASE + 4*(hartid))

#define RELEASE_CORE(hartid)        (*(uint32_t*)CLINT_MSIP(hartid)=1)

__SELF __INLINE uint64_t __clint_mtime_get()
{
    return *(uint64_t*)CLINT_MTIME;
}

__SELF __INLINE void __clint_mtimecmp_set(uint32_t hartid,uint64_t value)
{
    *(uint64_t*)CLINT_MTIMECMP(hartid) = value;
}

#endif 