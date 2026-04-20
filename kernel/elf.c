/**
 * @FilePath: /vboot/elf.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-07-21 23:53:30
 * @LastEditTime: 2025-09-17 23:40:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/printk.h>
#include <os/elf.h>
#include <os/kmalloc.h>
#include <os/check.h>
#include <os/string.h>

static int elf_check_image(const char *elf, size_t len)
{
    CHECK(elf != NULL, "elf is NULL", return -1;);
    CHECK(len >= sizeof(struct Elf32_Ehdr), "ELF image is too small", return -1;);

    const unsigned char *ident = (const unsigned char *)elf;

    CHECK(ident[0] == 0x7f && ident[1] == 'E' && ident[2] == 'L' && ident[3] == 'F',
          "Not a valid ELF file", return -1;);
    CHECK(ident[5] == ELFDATA2LSB, "Only little-endian ELF is supported", return -1;);
    CHECK(ident[4] == ELFCLASS32 || ident[4] == ELFCLASS64, "Unsupported ELF class", return -1;);

    if (ident[4] == ELFCLASS32) {
        const struct Elf32_Ehdr *ehdr = (const struct Elf32_Ehdr *)elf;

        CHECK(len >= sizeof(*ehdr), "ELF32 header truncated", return -1;);
        CHECK(ehdr->e_type == ET_EXEC, "ELF32 image is not executable", return -1;);
        CHECK(ehdr->e_phoff != 0, "ELF32 program header table missing", return -1;);
        CHECK(ehdr->e_phnum != 0, "ELF32 has no program headers", return -1;);
        CHECK(ehdr->e_phentsize == sizeof(struct Elf32_Phdr), "Unexpected ELF32 phdr size", return -1;);
        CHECK(ehdr->e_entry != 0, "ELF32 entry is 0", return -1;);
        CHECK((uint64_t)ehdr->e_phoff + (uint64_t)ehdr->e_phnum * sizeof(struct Elf32_Phdr) <= len,
              "ELF32 program headers out of range", return -1;);
        return 0;
    }

    {
        const struct Elf64_Ehdr *ehdr = (const struct Elf64_Ehdr *)elf;

        CHECK(len >= sizeof(*ehdr), "ELF64 header truncated", return -1;);
        CHECK(ehdr->e_type == ET_EXEC, "ELF64 image is not executable", return -1;);
        CHECK(ehdr->e_phoff != 0, "ELF64 program header table missing", return -1;);
        CHECK(ehdr->e_phnum != 0, "ELF64 has no program headers", return -1;);
        CHECK(ehdr->e_phentsize == sizeof(struct Elf64_Phdr), "Unexpected ELF64 phdr size", return -1;);
        CHECK(ehdr->e_entry != 0, "ELF64 entry is 0", return -1;);
        CHECK(ehdr->e_phoff + (uint64_t)ehdr->e_phnum * sizeof(struct Elf64_Phdr) <= len,
              "ELF64 program headers out of range", return -1;);
    }

    return 0;
}

struct elf_info *elf_parse_image(const char *elf, size_t len)
{
    CHECK(elf != NULL, "elf is NULL", return NULL;);
    CHECK(elf_check_image(elf, len) == 0, "Invalid ELF file", return NULL;);

    struct elf_info *info = kmalloc(sizeof(struct elf_info));
    CHECK(info != NULL, "Failed to allocate memory for ELF info", return NULL;);
    memset(info, 0, sizeof(struct elf_info));
    info->data = (char *)elf;
    info->size = len;

    if (((const unsigned char *)elf)[4] == ELFCLASS32) {
        const struct Elf32_Ehdr *ehdr = (const struct Elf32_Ehdr *)elf;
        const struct Elf32_Phdr *phdr = (const struct Elf32_Phdr *)(elf + ehdr->e_phoff);

        CHECK(ehdr->e_phnum <= (sizeof(info->segs) / sizeof(info->segs[0])),
              "Too many ELF32 program headers", kfree(info); return NULL;);

        info->elf_class = ELFCLASS32;
        info->machine = ehdr->e_machine;
        info->entry = ehdr->e_entry;
        info->phnum = ehdr->e_phnum;

        for (int i = 0; i < ehdr->e_phnum; i++) {
            info->segs[i].type = phdr[i].p_type;
            info->segs[i].vaddr = phdr[i].p_vaddr;
            info->segs[i].filesz = phdr[i].p_filesz;
            info->segs[i].memsz = phdr[i].p_memsz;
            info->segs[i].offset = phdr[i].p_offset;
            info->segs[i].flags = phdr[i].p_flags;
        }
    } else {
        const struct Elf64_Ehdr *ehdr = (const struct Elf64_Ehdr *)elf;
        const struct Elf64_Phdr *phdr = (const struct Elf64_Phdr *)(elf + ehdr->e_phoff);

        CHECK(ehdr->e_phnum <= (sizeof(info->segs) / sizeof(info->segs[0])),
              "Too many ELF64 program headers", kfree(info); return NULL;);

        info->elf_class = ELFCLASS64;
        info->machine = ehdr->e_machine;
        info->entry = ehdr->e_entry;
        info->phnum = ehdr->e_phnum;

        for (int i = 0; i < ehdr->e_phnum; i++) {
            info->segs[i].type = phdr[i].p_type;
            info->segs[i].vaddr = phdr[i].p_vaddr;
            info->segs[i].filesz = phdr[i].p_filesz;
            info->segs[i].memsz = phdr[i].p_memsz;
            info->segs[i].offset = phdr[i].p_offset;
            info->segs[i].flags = phdr[i].p_flags;
        }
    }

    printk("ELF entry point: %xu, machine: %du, program header count: %du\n",
           info->entry, info->machine, info->phnum);
    return info;
}

struct elf_info *elf_parse(const char *elf)
{
    return elf_parse_image(elf, UINT_MAX);
}

