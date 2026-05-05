#include <os/elf.h>
#include <os/printk.h>

int arch_elf_check(const struct elf_info *info)
{
    if (info->elf_class != ELFCLASS64) {
        printk("elf: only ELF64 supported on riscv64\n");
        return -1;
    }
    if (info->machine != EM_RISCV) {
        printk("elf: only RISC-V ELF supported\n");
        return -1;
    }
    return 0;
}
