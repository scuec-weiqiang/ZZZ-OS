#include <fs/binfmt.h>
#include <fs/file.h>
#include <os/mm.h>
#include <mm/pgtbl.h>
#include <os/elf.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/pfn.h>
#include <os/kva.h>
#include <os/string.h>
#include <os/sched.h>
#include <asm/process.h>
#include <asm/ptrace.h>

static int elf_check_arch(const struct elf_info *info) {
    if(info->elf_class != ELFCLASS32){
         printk("elf: only ELF32 supported\n");
         return -1;
    }
    if(info->machine != EM_ARM){
        printk("elf: only ARM ELF supported\n");
        return -1;
    }
    return 0;
 }

static pgprot_t elf_segment_prot(uint32_t elf_flags)
{
    pgprot_t prot = PROT_USER;

    if (elf_flags & PF_R) {
        prot |= PROT_READ;
    }
    if (elf_flags & PF_W) {
        prot |= PROT_WRITE;
    }
    if (elf_flags & PF_X) {
        prot |= PROT_EXEC;
    }
    dprintk("elf: segment flags=%xu -> prot=%xu\n", elf_flags, prot);
    return prot;
}

static void mm_record_segment(struct mm_struct *mm, const struct elf_segment *seg)
{
    uint64_t seg_start = seg->vaddr;
    uint64_t seg_end = seg->vaddr + seg->memsz;

    if (seg->flags & PF_X) {
        if (mm->start_code == 0 || seg_start < mm->start_code) {
            mm->start_code = seg_start;
        }
        if (seg_end > mm->end_code) {
            mm->end_code = seg_end;
        }
    } else {
        if (mm->start_data == 0 || seg_start < mm->start_data) {
            mm->start_data = seg_start;
        }
        if (seg_end > mm->end_data) {
            mm->end_data = seg_end;
        }
    }

    if (seg_end > mm->brk) {
        mm->brk = ALIGN_UP(seg_end, PAGE_SIZE);
        mm->start_brk = mm->brk;
    }
}

static int elf_map_segment(struct linux_binprm *bprm,
                           struct mm_struct *mm,
                           const struct elf_segment *seg,
                           size_t file_size)
{
    pgprot_t prot;
    uint64_t seg_start;
    uint64_t seg_end;
    uint64_t file_end;
    uintptr_t addr;

    if (seg->type != PT_LOAD || seg->memsz == 0) {
        here;
        return 0;
    }

    CHECK(seg->filesz <= seg->memsz, "elf: filesz > memsz", return -1;);
    CHECK((seg->vaddr & (PAGE_SIZE - 1)) == (seg->offset & (PAGE_SIZE - 1)),
          "elf: vaddr/offset alignment mismatch", return -1;);

    file_end = seg->offset + seg->filesz;
    CHECK(file_end <= file_size, "elf: segment exceeds file size", return -1;);

    seg_start = ALIGN_DOWN(seg->vaddr, PAGE_SIZE);
    seg_end = ALIGN_UP(seg->vaddr + seg->memsz, PAGE_SIZE);
    prot = elf_segment_prot(seg->flags);

    if (vma_add(mm, seg_start, seg_end - seg_start, prot) < 0) {
        return -1;
    }

    for (addr = seg_start; addr < seg_end; addr += PAGE_SIZE) {
        uint64_t data_start;
        uint64_t data_end;
        void *kva;
        size_t copy_len;
        size_t page_off;
        loff_t file_off;

        kva = page_alloc(1);
        CHECK(kva != NULL, "elf: page_alloc failed", return -1;);
        memset(kva, 0, PAGE_SIZE);

        if (map(mm->pgdir, addr, KERNEL_PA(kva), PAGE_SIZE, prot) < 0) {
            return -1;
        }

        data_start = addr > seg->vaddr ? addr : seg->vaddr;
        data_end = (addr + PAGE_SIZE) < (seg->vaddr + seg->filesz)
            ? (addr + PAGE_SIZE) : (seg->vaddr + seg->filesz);
        if (data_end <= data_start) {
            continue;
        }

        page_off = data_start - addr;
        copy_len = data_end - data_start;
        file_off = seg->offset + (data_start - seg->vaddr);

        if (kernel_read_at(bprm->file, file_off, (char *)kva + page_off, copy_len) != (ssize_t)copy_len) {
            return -1;
        }
    }

    mm_record_segment(mm, seg);
    return 0;
}

/*
 * 简化版 load_elf_binary：
 * 1. 解析 ELF header + program headers
 * 2. 把所有 PT_LOAD 映射到 bprm->mm
 * 3. 成功后提交新 mm
 * 4. 设置 pt_regs，准备进入用户态
 */
static int load_elf_binary(struct linux_binprm *bprm) {
    struct elf_info *elf_info;
    struct pt_regs *regs;
    int ret;
    int i;

    CHECK(bprm != NULL, "elf: bprm is NULL", return -1;);
    CHECK(bprm->file != NULL, "elf: file is NULL", return -1;);
    CHECK(bprm->file->f_inode != NULL, "elf: inode is NULL", return -1;);
    CHECK(bprm->mm != NULL, "elf: bprm->mm is NULL", return -1;);

    elf_info = elf_parse_file(bprm->file);
    CHECK(elf_info != NULL, "elf: parse header failed", return -1;);

    ret = elf_check_arch(elf_info);
    if (ret != 0) {
        goto out_fail;
    }

    dprintk("entry=%xu, phnum=%du, size=%du\n",
           elf_info->entry, elf_info->phnum, (int)elf_info->file_size);

    for (i = 0; i < elf_info->phnum; i++) {
        ret = elf_map_segment(bprm, bprm->mm, &elf_info->segs[i], elf_info->file_size);
        if (ret < 0) {
            goto out_fail;
        }
    }

    ret = flush_old_exec(bprm);
    if (ret < 0) {
        goto out_fail;
    }

    current->mm->start_stack = bprm->p;
    regs = task_pt_regs(current);

    {
        phys_addr_t pa = pgtbl_lookup(current->mm->pgdir, elf_info->entry);
        pgprot_t prot = pgtbl_lookup_prot(current->mm->pgdir, elf_info->entry);
        pte_t *root = (pte_t *)current->mm->pgdir->root;
        int l0 = pgtbl_level_index(current->mm->pgdir, 0, elf_info->entry);

        dprintk("exec: root=%xu root_pa=%xu\n", current->mm->pgdir->root, current->mm->pgdir->root_pa);
        dprintk("exec: entry=%xu lookup_pa=%xu prot=%xu l0=%du l0_pte=%xu\n",
        elf_info->entry, pa, prot, l0, root[l0].val);

    }

    start_thread(regs, elf_info->entry, bprm->p);

    printk("elf: loaded, pc=%xu, sp=%xu\n", regs->pc, regs->sp);

    elf_free(elf_info);
    return 0;

out_fail:
    if (bprm->mm) {
        mm_destroy(bprm->mm);
        bprm->mm = NULL;
    }
    elf_free(elf_info);
    return ret < 0 ? ret : -1;
}

static struct linux_binfmt elf_format = {
    .load_binary = load_elf_binary,
};

void elf_binfmt_init(void) {
    register_binfmt(&elf_format);
    printk("elf: binfmt registered\n");
}
