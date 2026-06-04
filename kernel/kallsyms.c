#include <os/kallsyms.h>

const char *lookup_symbol_name(unsigned long addr, unsigned long *offset)
{
    const struct kernel_symbol *best = NULL;
    unsigned int i;

    for (i = 0; i < __kallsyms_count; i++) {
        if (__kallsyms[i].addr > addr) {
            break;
        }
        best = &__kallsyms[i];
    }

    if (best == NULL) {
        return NULL;
    }

    if (offset != NULL) {
        *offset = addr - best->addr;
    }

    return best->name;
}
