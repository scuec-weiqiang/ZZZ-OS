#ifndef __OS_KALLSYMS_H
#define __OS_KALLSYMS_H

#include <os/types.h>

struct kernel_symbol {
    unsigned long addr;
    const char *name;
};

extern const struct kernel_symbol __kallsyms[];
extern const unsigned int __kallsyms_count;

const char *lookup_symbol_name(unsigned long addr, unsigned long *offset);

#endif
