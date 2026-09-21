#include <fs/file.h>
#include <os/check.h>
#include <os/elf.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>

static int elf_check_image(struct file *file, unsigned char ident[EI_NIDENT])
{
    ssize_t nread;

    if (!file) {
        printk("%s\n", "elf: file is NULL");
        return -1;
    }
    if (!file->f_inode) {
        printk("%s\n", "elf: inode is NULL");
        return -1;
    }
    if (!(file->f_inode->i_size >= sizeof(struct Elf32_Ehdr))) {
        printk("%s\n", "elf: image too small");
        return -1;
    }

    nread = kernel_read_at(file, 0, (char *)ident, EI_NIDENT);
    if (!(nread == EI_NIDENT)) {
        printk("%s\n", "elf: failed to read ident");
        return -1;
    }

    if (!(ident[0] == 0x7f && ident[1] == 'E' && ident[2] == 'L' && ident[3] == 'F')) {
        printk("%s\n", "elf: bad magic");
        return -1;
    }
    if (!(ident[5] == ELFDATA2LSB)) {
        printk("%s\n", "elf: only little-endian supported");
        return -1;
    }
    if (!(ident[4] == ELFCLASS32 || ident[4] == ELFCLASS64)) {
        printk("%s\n", "elf: unsupported class");
        return -1;
    }

    return 0;
}

void dump_elf32_ehdr(const struct Elf32_Ehdr *ehdr)
{
    dprintk("Elf32_Ehdr:\n");
    dprintk("  e_type: %xu\n", ehdr->e_type);
    dprintk("  e_machine: %xu\n", ehdr->e_machine);
    dprintk("  e_version: %xu\n", ehdr->e_version);
    dprintk("  e_entry: %xu\n", ehdr->e_entry);
    dprintk("  e_phoff: %xu\n", ehdr->e_phoff);
    dprintk("  e_shoff: %xu\n", ehdr->e_shoff);
    dprintk("  e_flags: %xu\n", ehdr->e_flags);
    dprintk("  e_ehsize: %xu\n", ehdr->e_ehsize);
    dprintk("  e_phentsize: %xu\n", ehdr->e_phentsize);
    dprintk("  e_phnum: %xu\n", ehdr->e_phnum);
    dprintk("  e_shentsize: %xu\n", ehdr->e_shentsize);
    dprintk("  e_shnum: %xu\n", ehdr->e_shnum);
    dprintk("  e_shstrndx: %xu\n", ehdr->e_shstrndx);
}

static int elf_load32(struct file *file, struct elf_info *info)
{
    struct Elf32_Ehdr ehdr;
    struct Elf32_Phdr *phdrs = NULL;
    size_t phdr_bytes;
    ssize_t nread;
    int i;

    nread = kernel_read_at(file, 0, (char *)&ehdr, sizeof(ehdr));
    if (!(nread == sizeof(ehdr))) {
        printk("%s\n", "elf32: header truncated");
        return -1;
    }

    // dump_elf32_ehdr(&ehdr);

    if (!(ehdr.e_type == ET_EXEC)) {
        printk("%s\n", "elf32: only ET_EXEC supported");
        return -1;
    }
    if (!ehdr.e_phoff) {
        printk("%s\n", "elf32: missing phdr table");
        return -1;
    }
    if (!ehdr.e_phnum) {
        printk("%s\n", "elf32: no program headers");
        return -1;
    }
    if (!(ehdr.e_phentsize == sizeof(struct Elf32_Phdr))) {
        printk("%s\n", "elf32: unexpected phdr size");
        return -1;
    }
    if (!ehdr.e_entry) {
        printk("%s\n", "elf32: entry is 0");
        return -1;
    }

    phdr_bytes = (size_t)ehdr.e_phnum * sizeof(struct Elf32_Phdr);
    if (!((u64)ehdr.e_phoff + phdr_bytes <= file->f_inode->i_size)) {
        printk("%s\n", "elf32: phdr table out of range");
        return -1;
    }

    phdrs = kmalloc(phdr_bytes);
    if (!phdrs) {
        printk("%s\n", "elf32: alloc phdrs failed");
        return -1;
    }

    nread = kernel_read_at(file, ehdr.e_phoff, (char *)phdrs, phdr_bytes);
    if (nread != (ssize_t)phdr_bytes) {
        kfree(phdrs);
        if (!(0)) {
            printk("%s\n", "elf32: failed to read phdrs");
            return -1;
        }
    }

    info->elf_class = ELFCLASS32;
    info->machine = ehdr.e_machine;
    info->entry = ehdr.e_entry;
    info->phnum = ehdr.e_phnum;
    info->file_size = file->f_inode->i_size;
    info->segs = kmalloc(sizeof(*info->segs) * info->phnum);
    if (!info->segs) {
        kfree(phdrs);
        return -1;
    }
    memset(info->segs, 0, sizeof(*info->segs) * info->phnum);

    for (i = 0; i < info->phnum; i++) {
        info->segs[i].type = phdrs[i].p_type;
        info->segs[i].vaddr = phdrs[i].p_vaddr;
        info->segs[i].filesz = phdrs[i].p_filesz;
        info->segs[i].memsz = (u64)phdrs[i].p_memsz;
        info->segs[i].offset = phdrs[i].p_offset;
        info->segs[i].flags = phdrs[i].p_flags;
        // dprintk("phdr[%d]: type=%xu vaddr=%xu filesz=%xu memsz=%xu offset=%xu flags=%xu\n",
        //        i, phdrs[i].p_type, phdrs[i].p_vaddr, phdrs[i].p_filesz,
        //        phdrs[i].p_memsz, phdrs[i].p_offset, phdrs[i].p_flags);
    }

    kfree(phdrs);
    return 0;
}

static int elf_load64(struct file *file, struct elf_info *info)
{
    struct Elf64_Ehdr ehdr;
    struct Elf64_Phdr *phdrs = NULL;
    size_t phdr_bytes;
    ssize_t nread;
    int i;

    nread = kernel_read_at(file, 0, (char *)&ehdr, sizeof(ehdr));
    if (!(nread == sizeof(ehdr))) {
        printk("%s\n", "elf64: header truncated");
        return -1;
    }

    if (!(ehdr.e_type == ET_EXEC)) {
        printk("%s\n", "elf64: only ET_EXEC supported");
        return -1;
    }
    if (!ehdr.e_phoff) {
        printk("%s\n", "elf64: missing phdr table");
        return -1;
    }
    if (!ehdr.e_phnum) {
        printk("%s\n", "elf64: no program headers");
        return -1;
    }
    if (!(ehdr.e_phentsize == sizeof(struct Elf64_Phdr))) {
        printk("%s\n", "elf64: unexpected phdr size");
        return -1;
    }
    if (!ehdr.e_entry) {
        printk("%s\n", "elf64: entry is 0");
        return -1;
    }
    if (!(ehdr.e_phoff + (u64)ehdr.e_phnum * sizeof(struct Elf64_Phdr) <= file->f_inode->i_size)) {
        printk("%s\n", "elf64: phdr table out of range");
        return -1;
    }

    phdr_bytes = (size_t)ehdr.e_phnum * sizeof(struct Elf64_Phdr);
    phdrs = kmalloc(phdr_bytes);
    if (!phdrs) {
        printk("%s\n", "elf64: alloc phdrs failed");
        return -1;
    }

    nread = kernel_read_at(file, ehdr.e_phoff, (char *)phdrs, phdr_bytes);
    if (nread != (ssize_t)phdr_bytes) {
        kfree(phdrs);
        if (!(0)) {
            printk("%s\n", "elf64: failed to read phdrs");
            return -1;
        }
    }

    info->elf_class = ELFCLASS64;
    info->machine = ehdr.e_machine;
    info->entry = ehdr.e_entry;
    info->phnum = ehdr.e_phnum;
    info->file_size = file->f_inode->i_size;
    info->segs = kmalloc(sizeof(*info->segs) * info->phnum);
    if (!info->segs) {
        kfree(phdrs);
        return -1;
    }
    memset(info->segs, 0, sizeof(*info->segs) * info->phnum);

    for (i = 0; i < info->phnum; i++) {
        info->segs[i].type = phdrs[i].p_type;
        info->segs[i].vaddr = phdrs[i].p_vaddr;
        info->segs[i].filesz = phdrs[i].p_filesz;
        info->segs[i].memsz = phdrs[i].p_memsz;
        info->segs[i].offset = phdrs[i].p_offset;
        info->segs[i].flags = phdrs[i].p_flags;
    }

    kfree(phdrs);
    return 0;
}

struct elf_info *elf_parse_file(struct file *file)
{
    struct elf_info *info;
    unsigned char ident[EI_NIDENT];
    int ret;

    ret = elf_check_image(file, ident);
    if (ret < 0) {
        return NULL;
    }

    info = kmalloc(sizeof(*info));
    if (!info) {
        printk("%s\n", "elf: alloc info failed");
        return NULL;
    }
    memset(info, 0, sizeof(*info));

    if (ident[4] == ELFCLASS32) {
        ret = elf_load32(file, info);
    } else {
        ret = elf_load64(file, info);
    }

    if (ret < 0) {
        elf_free(info);
        return NULL;
    }

    // dprintk("entry=%xu, machine=%xu, phnum=%xu\n",
    //        info->entry, info->machine, info->phnum);
    return info;
}

void elf_free(struct elf_info *info)
{
    if (!info) {
        return;
    }

    if (info->segs) {
        kfree(info->segs);
        info->segs = NULL;
    }

    kfree(info);
}
