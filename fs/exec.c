#include <fs/binfmt.h>
#include <fs/file.h>
#include <os/kmalloc.h>
#include <os/check.h>
#include <os/list.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/err.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/kva.h>

#include <mm/pgtbl.h>
#include <asm/ptrace.h>

static LIST_HEAD(formats);

int register_binfmt(struct linux_binfmt *fmt) {
    CHECK(fmt != NULL, "exec: invalid binfmt", return -1;);
    CHECK(fmt->load_binary != NULL, "exec: binfmt missing load_binary", return -1;);

    INIT_LIST_HEAD(&fmt->lh);
    list_add(&formats, &fmt->lh);
    // dprintk("exec: registered binfmt\n");
    return 0;
}

int unregister_binfmt(struct linux_binfmt *fmt) {
    if (fmt != NULL && !list_empty(&fmt->lh)) {
        list_del(&fmt->lh);
        INIT_LIST_HEAD(&fmt->lh);
    }
    return 0;
}

void set_binfmt(struct linux_binfmt *new) {
    /* 简化版：仅记录当前进程的 binfmt 类型（可选扩展 task_struct） */
    (void)new;
}


/*
 * count() counts the number of strings in array ARGV.
 */
static int count(char** argv, int max) {
     int i = 0;
 
     if (argv != NULL) {
         for (;;) {
            char __user *p = argv[i];
 
             if (!p)
                 break;
 
             if (IS_ERR(p))
                 return -EFAULT;
 
             if (i >= max)
                 return -E2BIG;
             ++i;
         }
     }
     return i;
 }

static int valid_arg_len(struct linux_binprm *bprm, size_t len) {
    unsigned long stack_base = USER_STACK_TOP - USER_STACK_SIZE;

    if (!bprm || !bprm->mm) {
        return 0;
    }

    if (len == 0 || len > MAX_ARG_STRLEN) {
        return 0;
    }

    if (bprm->p < stack_base + len) {
        return 0;
    }

    return 1;
}

static int copy_string_to_stack(struct linux_binprm *bprm,
                                unsigned long dst,
                                const char *src,
                                size_t len) {
    size_t copied = 0;

    while (copied < len) {
        unsigned long va = dst + copied;
        unsigned long page_off = va & (PAGE_SIZE - 1);
        size_t chunk = PAGE_SIZE - page_off;
        phys_addr_t pa;
        void *kva;

        if (chunk > len - copied) {
            chunk = len - copied;
        }

        pa = pgtbl_lookup(bprm->mm->pgdir, va);
        if (!pa) {
            return -EFAULT;
        }

        kva = (void *)KERNEL_VA(pa);
        memcpy((char *)kva, src + copied, chunk);
        copied += chunk;
    }

    return 0;
}

/*
    把 argv 字符串真正压入到新进程的用户栈中。
    栈向低地址增长，所以按 argc-1 -> 0 的顺序拷贝，
    每拷入一个字符串都下移 bprm->p。
 */
static int copy_strings(int argc, char **argv, struct linux_binprm *bprm) {
    int i;

    if (!bprm) {
        return -EINVAL;
    }

    if (argc == 0 || !argv) {
        return 0;
    }

    for (i = argc - 1; i >= 0; i--) {
        const char *str = argv[i];
        size_t len;
        unsigned long dst;
        int ret;

        if (!str) {
            return -EFAULT;
        }

        len = strlen(str) + 1;
        if (!valid_arg_len(bprm, len)) {
            return -E2BIG;
        }

        bprm->p -= len;
        dst = bprm->p;

        ret = copy_string_to_stack(bprm, dst, str, len);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

/*
    在已经写入字符串的用户栈上补齐启动布局。
    最终布局从低地址到高地址为：
    argc
    argv[0..argc-1]
    NULL
    envp[0] = NULL
    argv 字符串区
 */
static int create_user_stack_layout(int argc, char **argv, struct linux_binprm *bprm) {
    unsigned long *argv_user = NULL;
    unsigned long *stack_words = NULL;
    unsigned long cursor;
    unsigned long argc_val = argc;
    size_t total_words;
    size_t total_bytes;
    size_t pad_words;
    int ret = 0;
    int i;
    int idx = 0;

    if (!bprm || !bprm->mm) {
        return -EINVAL;
    }

    argv_user = kmalloc(sizeof(unsigned long) * (argc + 1));
    if (!argv_user) {
        return -ENOMEM;
    }

    cursor = bprm->p;
    for (i = 0; i < argc; i++) {
        if (!argv || !argv[i]) {
            ret = -EFAULT;
            goto out;
        }
        argv_user[i] = cursor;
        cursor += strlen(argv[i]) + 1;
    }
    argv_user[argc] = 0;

    /*
     * AAPCS requires SP to be 8-byte aligned at a public interface.
     * Newlib's varargs code relies on this.
     */
    bprm->p = ALIGN_DOWN(bprm->p, 8);

    /*
     *   栈布局
     *   argc
     *   argv[0..argc-1]
     *   NULL
     *   envp[0] = NULL
     *   optional padding word to keep SP 8-byte aligned
     */
    total_words = 1 + (argc + 1) + 1;
    pad_words = total_words & 0x1;
    total_words += pad_words;
    total_bytes = total_words * sizeof(unsigned long);

    stack_words = kmalloc(total_bytes);
    if (!stack_words) {
        ret = -ENOMEM;
        goto out;
    }
    memset(stack_words, 0, total_bytes);

    stack_words[idx++] = argc_val;
    for (i = 0; i < argc; i++) {
        stack_words[idx++] = argv_user[i];
    }
    stack_words[idx++] = 0;
    stack_words[idx++] = 0;

    bprm->p -= total_bytes;
    ret = copy_string_to_stack(bprm, bprm->p, (const char *)stack_words, total_bytes);
    if (ret < 0) {
        goto out;
    }

    bprm->mm->start_stack = bprm->p;

out:
    kfree(stack_words);
    kfree(argv_user);
    return ret;
}

/*
    search_binary_handler - 遍历已注册的 binfmt 链表，
    依次调用 load_binary 直到有一个识别当前文件
 */
int search_binary_handler(struct linux_binprm *bprm) {
    struct linux_binfmt *fmt;
    int ret;

    CHECK(bprm != NULL, "exec: bprm is NULL", return -EINVAL;);

    list_for_each_entry(fmt, &formats, struct linux_binfmt, lh) {
        ret = fmt->load_binary(bprm);
        if (ret == 0) {
            set_binfmt(fmt);
            return 0;
        }
    }

    return -ENOEXEC;
}


static int setup_arg_pages(struct linux_binprm *bprm) {
    struct mm_struct *mm = bprm->mm;
    virt_addr_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;
    pgprot_t prot = PROT_USER | PROT_READ | PROT_WRITE;

    if (!mm)
        return -EINVAL;

    int ret = vma_add(mm, stack_base, USER_STACK_SIZE, prot);
    if (ret < 0){
        return ret;
    }
        

    for (int i = 0; i < USER_STACK_PAGES; i++) {
        void *kva = page_alloc(1);
        if (!kva)
            return -ENOMEM;

        memset(kva, 0, PAGE_SIZE);

        int ret = map(mm->pgdir,stack_base + i * PAGE_SIZE,KERNEL_PA(kva),PAGE_SIZE,prot);
          
        if (ret < 0) {
            kfree(kva);
            return ret;
        }
        
    }

    mm->start_stack = USER_STACK_TOP;
    bprm->p = USER_STACK_TOP;
    return 0;
}

static void clear_arg_pages(struct linux_binprm *bprm) {
    struct mm_struct *mm = bprm->mm;
    virt_addr_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;

    if (!mm)
        return;

    for (int i = 0; i < USER_STACK_PAGES; i++) {
        virt_addr_t va = stack_base + i * PAGE_SIZE;
        phys_addr_t pa = pgtbl_lookup(mm->pgdir, va);
        if (pa) {
            unmap(mm->pgdir, va, PAGE_SIZE);
            kfree((void *)KERNEL_VA(pa));
        }
    }

    vma_delete(mm, stack_base, USER_STACK_SIZE);
}

static int bprm_mm_init(struct linux_binprm *bprm) {
	int err;
	struct mm_struct *mm = NULL;

	bprm->mm = mm = mm_alloc();
	err = -ENOMEM;
	if (!mm)
		return err;

    copy_kernel_mapping(mm);

    err = setup_arg_pages(bprm);
    if (err < 0) {
 
        mm_destroy(mm);
        bprm->mm = NULL;
        return err;
    }
    
	return 0;
}

static void bprm_mm_deinit(struct linux_binprm *bprm) {
    if (!bprm || !bprm->mm) {
        return;
    }

    clear_arg_pages(bprm);
    mm_destroy(bprm->mm);
    bprm->mm = NULL;
}

int flush_old_exec(struct linux_binprm *bprm) {
    struct mm_struct *old_mm;

    if (!bprm || !bprm->mm) {
        return -EINVAL;
    }

    old_mm = current->mm;

    current->mm = bprm->mm;
    current->active_mm = bprm->mm;
    bprm->mm = NULL;

    pgtbl_switch_to(current->active_mm->pgdir);
    pgtbl_flush();

    current->flags &= ~PF_KTHREAD;

    if (old_mm && old_mm != &init_mm && old_mm != current->mm) {
        mm_destroy(old_mm);
    }

    return 0;
}

int do_execve(char *filename, char* argv[], char* envp[]) {
    struct linux_binprm *bprm = NULL;
    struct file *file;
    int retval = -ENOMEM;

    if (!filename) {
        return -EINVAL;
    }

    bprm = kzalloc(sizeof(struct linux_binprm));
    if (!bprm) {
        retval = -ENOMEM;
        goto failed;
    }

    current->in_execve = 1;

    do_close_on_exec(current->files);

    file = filp_open(filename, O_RDONLY);
    if (IS_ERR(file)) {
        retval = PTR_ERR(file);
        goto open_failed;
    }

    bprm->file = file;
    bprm->filename = filename;
    bprm->interp = filename;

    retval = bprm_mm_init(bprm);
    if (retval < 0) {
        goto mm_failed;
    }
  
    bprm->argc = count(argv, MAX_ARG_STRINGS);
    if (bprm->argc < 0) {
        retval = bprm->argc;
        goto count_failed;
    }

    memset(bprm->buf, 0, BINPRM_BUF_SIZE);
    kernel_read(bprm->file, bprm->buf, BINPRM_BUF_SIZE);

    retval = copy_strings(bprm->argc, argv, bprm);
    if (retval < 0) {
        goto copy_failed;
    }

    retval = create_user_stack_layout(bprm->argc, argv, bprm);
    if (retval < 0) {
        goto create_failed;
    }
    here;
    retval = search_binary_handler(bprm);
    if (retval < 0) {
        goto bin_failed;
    }
   
    return retval;

bin_failed:
create_failed:
copy_failed:
count_failed:
    bprm_mm_deinit(bprm);
mm_failed:
    filp_close(bprm->file);
open_failed:
    // free_files_struct(displaced);
// files_failed:
    kfree(bprm);
failed:
printk("err retval=%d\n", retval);
    return retval;
}

int sys_execve(struct pt_regs *ctx) {
    char *filename = (char *)ctx->r[0];
    char **argv = (char **)ctx->r[1];
    char **envp = (char **)ctx->r[2];

    return do_execve(filename, argv, envp);
}