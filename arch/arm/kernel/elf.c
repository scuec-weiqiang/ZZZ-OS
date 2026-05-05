#include <os/elf.h>
#include <os/printk.h>

int arch_elf_check(const struct elf_info *info)
{
    if (info->elf_class != ELFCLASS32) {
        printk("elf: only ELF32 supported on ARM\n");
        return -1;
    }
    if (info->machine != EM_ARM) {
        printk("elf: only ARM ELF supported\n");
        return -1;
    }
    return 0;
}

