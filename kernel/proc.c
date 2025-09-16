#include "proc.h"
#include "vfs.h"
#include "elf.h"
#include "vm.h"
#include "check.h"

pid_t alloc_pid()
{
    static pid_t next_pid = 1;
    return next_pid++;
}

proc_t* proc_create(char* path)
{
    CHECK(path != NULL, "proc create failed,path is NULL", return NULL;);

    vfs_file_t* f = vfs_open(path,0);
    char* elf = malloc(f->f_inode->i_size);
    ssize_t ret =  vfs_read(f,elf,f->f_inode->i_size);
    CHECK(ret >= 0, "vfs read failed", free(elf); return NULL;);

    elf_info_t *elf_info = elf_parse(elf);
    CHECK(elf_info != NULL, "elf parse failed", free(elf); return NULL;);

    proc_t* new_proc = malloc(sizeof(proc_t));
    memset(new_proc,0,sizeof(proc_t));
    
    pgtbl_t* user_pgd = page_alloc(1);
    memset(user_pgd,0,sizeof(pgtbl_t));

    uint8_t *user_stack = malloc(PROC_STACK_SIZE);
    memset(user_stack,0,PROC_STACK_SIZE);

    uint8_t *kernel_stack = malloc(PROC_STACK_SIZE);
    memset(kernel_stack,0,PROC_STACK_SIZE);

    

    for(int i=0;i<elf_info->phnum;i++)
    {
        if(elf_info->segs[i].type== PT_LOAD)
        {
            printf("phdr %d: vaddr:%x, memsz:%x, filesz:%x, offset:%x, flags:%x\n",i,elf_info->segs[i].vaddr,elf_info->segs[i].memsz,elf_info->segs[i].filesz,elf_info->segs[i].offset,elf_info->segs[i].flags);
            uint8_t *user_space = malloc(elf_info->segs[i].memsz); //程序加载到内存里需要的空间
            memset(user_space,0,elf_info->segs[i].memsz);
            memcpy(user_space,elf+elf_info->segs[i].offset,elf_info->segs[i].filesz);
            map_pages(user_pgd, elf_info->segs[i].vaddr, (uintptr_t)user_space, (elf_info->segs[i].memsz+PAGE_SIZE-1)/PAGE_SIZE * PAGE_SIZE, PTE_R|PTE_X|PTE_U);
        }
    }
    map_pages(user_pgd, PROC_USER_STACK_BOTTOM, (uintptr_t)user_stack, PROC_STACK_SIZE,  PTE_W|PTE_R|PTE_U);
    page_table_init(user_pgd);

    new_proc->elf_info = elf_info;
    new_proc->user_sp = PROC_USER_STACK_TOP;
    new_proc->kernel_sp = (uintptr_t)kernel_stack + PROC_STACK_SIZE - sizeof(reg_context_t);
    new_proc->pgd = user_pgd;
    new_proc->context.sepc = elf_info->entry;
    new_proc->context.sp = new_proc->user_sp;
    new_proc->pid = alloc_pid();
    new_proc->status = 0;

    return new_proc;
}

void proc_run(proc_t *p)
{
    if(p == NULL) return ;
    sscratch_w(sp_r());
    satp_w(MAKE_SATP(p->pgd));
    sstatus_w(sstatus_r() & ~(1<<8));  
    sepc_w((uintptr_t)(p->context.sepc)); 
    asm volatile("mv sp,%0"::"r"(p->user_sp));
    asm volatile("sret");
}