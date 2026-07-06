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
#include <os/uaccess.h>
#include <os/elf.h>

#include <mm/pgtbl.h>
#include <asm/ptrace.h>
#include <asm/signal.h>

static LIST_HEAD(formats);
#define EXEC_PATH_MAX 256

#define AT_NULL     0
#define AT_PHDR     3
#define AT_PHENT    4
#define AT_PHNUM    5
#define AT_PAGESZ   6
#define AT_ENTRY    9
#define AT_UID      11
#define AT_EUID     12
#define AT_GID      13
#define AT_EGID     14
#define AT_SECURE   23
#define AT_RANDOM   25

#define ELF_AUX_ENTRIES 12
#define ELF_RANDOM_BYTES 16

static const char *exec_basename(const char *path)
{
    const char *base = path;

    if (path == NULL)
        return "";

    while (*path != '\0') {
        if (*path == '/' && path[1] != '\0')
            base = path + 1;
        path++;
    }

    return base;
}

static int prepare_elf_aux(struct linux_binprm *bprm)
{
    struct Elf64_Ehdr *ehdr;
    struct Elf64_Phdr *phdrs = NULL;
    size_t phdr_size;
    int i;
    int ret = 0;

    if (bprm == NULL)
        return -EINVAL;

    ehdr = (struct Elf64_Ehdr *)bprm->buf;
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        return 0;
    }

    if (ehdr->e_ident[4] != ELFCLASS64 ||
        ehdr->e_phentsize != sizeof(struct Elf64_Phdr) ||
        ehdr->e_phnum == 0) {
        return 0;
    }

    phdr_size = (size_t)ehdr->e_phentsize * ehdr->e_phnum;
    phdrs = kmalloc(phdr_size);
    if (phdrs == NULL)
        return -ENOMEM;

    if (kernel_read_at(bprm->file, ehdr->e_phoff, (char *)phdrs,
                       phdr_size) != (ssize_t)phdr_size) {
        ret = -EIO;
        goto out;
    }

    bprm->elf_entry = ehdr->e_entry;
    bprm->elf_phent = ehdr->e_phentsize;
    bprm->elf_phnum = ehdr->e_phnum;

    for (i = 0; i < ehdr->e_phnum; i++) {
        struct Elf64_Phdr *phdr = &phdrs[i];

        if (phdr->p_type != PT_LOAD)
            continue;

        if (ehdr->e_phoff >= phdr->p_offset &&
            ehdr->e_phoff < phdr->p_offset + phdr->p_filesz) {
            bprm->elf_phdr = phdr->p_vaddr + (ehdr->e_phoff - phdr->p_offset);
            break;
        }
    }

out:
    kfree(phdrs);
    return ret;
}

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

static void free_kargv(char **argv)
{
    int i;

    if (argv == NULL)
        return;

    for (i = 0; argv[i] != NULL; i++) {
        kfree(argv[i]);
    }

    kfree(argv);
}

static int dup_user_string(const char *user_str, size_t max_len, char **out)
{
    size_t len = 0;
    char ch;
    char *kstr;

    if (out == NULL)
        return -EINVAL;

    *out = NULL;
    if (user_str == NULL)
        return -EFAULT;

    for (;;) {
        if (len >= max_len)
            return -ENAMETOOLONG;

        if (copy_from_user(&ch, (char *)user_str + len, 1) < 0)
            return -EFAULT;

        len++;
        if (ch == '\0')
            break;
    }

    kstr = kmalloc(len);
    if (kstr == NULL)
        return -ENOMEM;

    if (copy_from_user(kstr, (char *)user_str, len) < 0) {
        kfree(kstr);
        return -EFAULT;
    }

    *out = kstr;
    return 0;
}

static int dup_user_argv(char *const *user_argv, char ***out)
{
    char **kargv;
    char *user_str;
    int argc = 0;
    int i;
    int ret;

    if (out == NULL)
        return -EINVAL;

    *out = NULL;
    if (user_argv == NULL)
        return 0;

    for (;;) {
        if (argc >= MAX_ARG_STRINGS)
            return -E2BIG;

        if (copy_from_user((char *)&user_str,
                           (char *)(user_argv + argc),
                           sizeof(user_str)) < 0)
            return -EFAULT;

        if (user_str == NULL)
            break;

        argc++;
    }

    kargv = kzalloc(sizeof(char *) * (argc + 1));
    if (kargv == NULL)
        return -ENOMEM;

    for (i = 0; i < argc; i++) {
        if (copy_from_user((char *)&user_str,
                           (char *)(user_argv + i),
                           sizeof(user_str)) < 0) {
            ret = -EFAULT;
            goto err;
        }

        ret = dup_user_string(user_str, MAX_ARG_STRLEN, &kargv[i]);
        if (ret < 0)
            goto err;
    }

    *out = kargv;
    return 0;

err:
    free_kargv(kargv);
    return ret;
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
static int copy_strings(int count,
                        char **strings,
                        struct linux_binprm *bprm)
{
    int i;

    if (!bprm)
        return -EINVAL;

    if (count <= 0 || !strings)
        return 0;

    /*
     * 栈向低地址增长，
     * 所以逆序压栈。
     */
    for (i = count - 1; i >= 0; i--) {

        const char *str = strings[i];
        size_t len;
        unsigned long dst;
        int ret;

        if (!str)
            return -EFAULT;

        len = strlen(str) + 1;

        if (!valid_arg_len(bprm, len))
            return -E2BIG;

        bprm->p -= len;

        dst = bprm->p;

        ret = copy_string_to_stack(bprm,
                                   dst,
                                   str,
                                   len);

        if (ret < 0)
            return ret;
    }

    return 0;
}
// static int copy_strings(int argc, char **argv, struct linux_binprm *bprm) {
//     int i;

//     if (!bprm) {
//         return -EINVAL;
//     }

//     if (argc == 0 || !argv) {
//         return 0;
//     }

//     for (i = argc - 1; i >= 0; i--) {
//         const char *str = argv[i];
//         size_t len;
//         unsigned long dst;
//         int ret;

//         if (!str) {
//             return -EFAULT;
//         }

//         len = strlen(str) + 1;
//         if (!valid_arg_len(bprm, len)) {
//             return -E2BIG;
//         }

//         bprm->p -= len;
//         dst = bprm->p;

//         ret = copy_string_to_stack(bprm, dst, str, len);
//         if (ret < 0) {
//             return ret;
//         }
//     }

//     return 0;
// }

/*
    在已经写入字符串的用户栈上补齐启动布局。
    最终布局从低地址到高地址为：
    argc
    argv[0..argc-1]
    NULL
    envp[0] = NULL
    argv 字符串区
 */
// static int create_user_stack_layout(int argc, char **argv, struct linux_binprm *bprm) {
//     unsigned long *argv_user = NULL;
//     unsigned long *stack_words = NULL;
//     unsigned long cursor;
//     unsigned long argc_val = argc;
//     size_t total_words;
//     size_t total_bytes;
//     size_t pad_words;
//     int ret = 0;
//     int i;
//     int idx = 0;

//     if (!bprm || !bprm->mm) {
//         return -EINVAL;
//     }

//     argv_user = kmalloc(sizeof(unsigned long) * (argc + 1));
//     if (!argv_user) {
//         return -ENOMEM;
//     }



//     cursor = bprm->p;
//     for (i = 0; i < argc; i++) {
//         if (!argv || !argv[i]) {
//             ret = -EFAULT;
//             goto out;
//         }
//         argv_user[i] = cursor;
//         cursor += strlen(argv[i]) + 1;
//     }
//     argv_user[argc] = 0;

//     /*
//      * Keep the initial userspace stack aligned to the target ABI.
//      * ARM newlib needs 8-byte alignment, while RV64 uses 16-byte alignment.
//      */
//     bprm->p = ALIGN_DOWN(bprm->p, USER_STACK_ALIGN);

//     /*
//      *   栈布局
//      *   argc
//      *   argv[0..argc-1]
//      *   NULL
//      *   envp[0] = NULL
//      *   optional padding words to keep SP ABI-aligned
//      */
//     total_words = 1 + (argc + 1) + 1;
//     pad_words = (USER_STACK_ALIGN / sizeof(unsigned long) -
//                  (total_words % (USER_STACK_ALIGN / sizeof(unsigned long)))) %
//                 (USER_STACK_ALIGN / sizeof(unsigned long));
//     total_words += pad_words;
//     total_bytes = total_words * sizeof(unsigned long);

//     stack_words = kmalloc(total_bytes);
//     if (!stack_words) {
//         ret = -ENOMEM;
//         goto out;
//     }
//     memset(stack_words, 0, total_bytes);

//     stack_words[idx++] = argc_val;
//     for (i = 0; i < argc; i++) {
//         stack_words[idx++] = argv_user[i];
//     }
//     stack_words[idx++] = 0;
//     stack_words[idx++] = 0;

//     bprm->p -= total_bytes;
//     ret = copy_string_to_stack(bprm, bprm->p, (const char *)stack_words, total_bytes);
//     if (ret < 0) {
//         goto out;
//     }

//     bprm->mm->start_stack = bprm->p;

// out:
//     kfree(stack_words);
//     kfree(argv_user);
//     return ret;
// }
static int create_user_stack_layout(int argc,
                                    char **argv,
                                    int envc,
                                    char **envp,
                                    struct linux_binprm *bprm)
{
    unsigned long *argv_user = NULL;
    unsigned long *envp_user = NULL;
    unsigned long *stack_words = NULL;

    unsigned long cursor;
    unsigned long random_user = 0;
    static const unsigned char random_bytes[ELF_RANDOM_BYTES] = {
        0x13, 0x57, 0x9b, 0xdf, 0x24, 0x68, 0xac, 0xe0,
        0x31, 0x75, 0xb9, 0xfd, 0x42, 0x86, 0xca, 0x0e,
    };

    size_t total_words;
    size_t total_bytes;
    size_t pad_words;

    int idx = 0;
    int i;
    int ret = 0;

    if (!bprm || !bprm->mm)
        return -EINVAL;

    /*
     * argv pointer array
     */
    argv_user = kmalloc(sizeof(unsigned long) * (argc + 1));

    if (!argv_user)
        return -ENOMEM;

    /*
     * envp pointer array
     */
    envp_user = kmalloc(sizeof(unsigned long) * (envc + 1));

    if (!envp_user) {
        ret = -ENOMEM;
        goto out;
    }

    /*
     * argv 字符串地址计算
     *
     * 注意：
     * copy_strings() 是逆序压栈，
     * 所以最终 argv[0] 位于最低地址。
     */
    cursor = bprm->arg_start;

    for (i = 0; i < argc; i++) {

        argv_user[i] = cursor;

        cursor += strlen(argv[i]) + 1;
    }

    argv_user[argc] = 0;

    /*
     * envp 字符串地址计算
     */
    cursor = bprm->env_start;

    for (i = 0; i < envc; i++) {

        envp_user[i] = cursor;

        cursor += strlen(envp[i]) + 1;
    }

    envp_user[envc] = 0;

    /*
     * Linux libc expects an auxiliary vector after envp.
     * AT_RANDOM points at 16 bytes placed in the initial stack area.
     */
    bprm->p -= ELF_RANDOM_BYTES;
    random_user = bprm->p;

    ret = copy_string_to_stack(bprm,
                               random_user,
                               (const char *)random_bytes,
                               ELF_RANDOM_BYTES);
    if (ret < 0)
        goto out;

    /*
     * ABI 对齐
     */
    bprm->p = ALIGN_DOWN(bprm->p,
                         USER_STACK_ALIGN);


    total_words =
        1 +              /* argc */
        (argc + 1) +     /* argv + NULL */
        (envc + 1) +     /* envp + NULL */
        (ELF_AUX_ENTRIES * 2);

    /*
     * 对齐 padding
     */
    pad_words =
        (USER_STACK_ALIGN / sizeof(unsigned long)
        - (total_words %
           (USER_STACK_ALIGN /
            sizeof(unsigned long))))
        %
        (USER_STACK_ALIGN /
         sizeof(unsigned long));

    total_words += pad_words;

    total_bytes =
        total_words *
        sizeof(unsigned long);

    stack_words = kmalloc(total_bytes);

    if (!stack_words) {
        ret = -ENOMEM;
        goto out;
    }

    memset(stack_words, 0, total_bytes);

    /*
     * argc
     */
    stack_words[idx++] = argc;

    /*
     * argv[]
     */
    for (i = 0; i < argc; i++) {
        stack_words[idx++] = argv_user[i];
    }

    stack_words[idx++] = 0;

    /*
     * envp[]
     */
    for (i = 0; i < envc; i++) {
        stack_words[idx++] = envp_user[i];
    }

    stack_words[idx++] = 0;

    /*
     * auxv[]
     *
     * This is intentionally minimal. It is enough for static libc startup
     * to stop scanning at AT_NULL and to discover the page size/random seed.
     */
    stack_words[idx++] = AT_PHDR;
    stack_words[idx++] = bprm->elf_phdr;
    stack_words[idx++] = AT_PHENT;
    stack_words[idx++] = bprm->elf_phent;
    stack_words[idx++] = AT_PHNUM;
    stack_words[idx++] = bprm->elf_phnum;
    stack_words[idx++] = AT_ENTRY;
    stack_words[idx++] = bprm->elf_entry;
    stack_words[idx++] = AT_PAGESZ;
    stack_words[idx++] = PAGE_SIZE;
    stack_words[idx++] = AT_UID;
    stack_words[idx++] = 0;
    stack_words[idx++] = AT_EUID;
    stack_words[idx++] = 0;
    stack_words[idx++] = AT_GID;
    stack_words[idx++] = 0;
    stack_words[idx++] = AT_EGID;
    stack_words[idx++] = 0;
    stack_words[idx++] = AT_SECURE;
    stack_words[idx++] = 0;
    stack_words[idx++] = AT_RANDOM;
    stack_words[idx++] = random_user;
    stack_words[idx++] = AT_NULL;
    stack_words[idx++] = 0;

    /*
     * 压入 pointer 区
     */
    bprm->p -= total_bytes;

    ret = copy_string_to_stack(bprm,
                               bprm->p,
                               (const char *)stack_words,
                               total_bytes);

    if (ret < 0)
        goto out;

    bprm->mm->start_stack = bprm->p;

out:

    kfree(stack_words);
    kfree(argv_user);
    kfree(envp_user);

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
    virt_addr_t sigtramp = USER_SIGTRAMP_ADDR;
    pgprot_t prot = PROT_USER | PROT_READ | PROT_WRITE;

    if (!mm)
        return -EINVAL;

    int ret = vma_add(mm, stack_base, USER_STACK_SIZE, prot);
    if (ret < 0){
        return ret;
    }
        

    // 分配用户栈页并映射
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

    // 分配信号处理 trampoline 页并映射
    {
        void *kva;
        pgprot_t sig_prot = PROT_USER | PROT_READ | PROT_EXEC;

        ret = vma_add(mm, sigtramp, PAGE_SIZE, sig_prot);
        if (ret < 0) {
            return ret;
        }

        kva = page_alloc(1);
        if (!kva) {
            return -ENOMEM;
        }
        memset(kva, 0, PAGE_SIZE);
        memcpy(kva, sigtramp_code, sizeof(sigtramp_code));

        ret = map(mm->pgdir, sigtramp, KERNEL_PA(kva), PAGE_SIZE, sig_prot);
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
    virt_addr_t sigtramp = USER_SIGTRAMP_ADDR;

    if (!mm)
        return;

    for (int i = 0; i < USER_STACK_PAGES; i++) {
        virt_addr_t va = stack_base + i * PAGE_SIZE;
        phys_addr_t pa = pgtbl_lookup(mm->pgdir, va);
        if (pa) {
            unmap(mm->pgdir, va, PAGE_SIZE);
            kfree((void *)KERNEL_VA(ALIGN_DOWN(pa, PAGE_SIZE)));
        }
    }

    vma_delete(mm, stack_base, USER_STACK_SIZE);

    {
        phys_addr_t pa = pgtbl_lookup(mm->pgdir, sigtramp);
        if (pa) {
            unmap(mm->pgdir, sigtramp, PAGE_SIZE);
            kfree((void *)KERNEL_VA(ALIGN_DOWN(pa, PAGE_SIZE)));
        }
    }
    vma_delete(mm, sigtramp, PAGE_SIZE);
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

#include <os/completion.h>

int flush_old_exec(struct linux_binprm *bprm) {
    if (!bprm || !bprm->mm) {
        return -EINVAL;
    }

    task_set_mm(current, bprm->mm);
    bprm->mm = NULL;

    pgtbl_switch_to(current->active_mm->pgdir);
    pgtbl_flush();

    if (current->vfork_done) {
        complete(current->vfork_done);
        current->vfork_done = NULL;
    }

    current->flags &= ~PF_KTHREAD;
    current->signal_trampoline = 0;
    current->set_child_tid = NULL;
    current->clear_child_tid = NULL;

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
    
    file = filp_open(filename, O_RDONLY);
    if (IS_ERR(file)) {
        retval = PTR_ERR(file);
        goto open_failed;
    }
    
    bprm->file = file;
    bprm->filename = filename;
    bprm->interp = filename;
    
    // 赋值内核页表，并分配用户栈页和信号 trampoline 页
    retval = bprm_mm_init(bprm);
    if (retval < 0) {
        goto mm_failed;
    }
    
    bprm->argc = count(argv, MAX_ARG_STRINGS);
    if (bprm->argc < 0) {
        retval = bprm->argc;
        goto count_failed;
    }

    bprm->envc = count(envp, MAX_ARG_STRINGS);
    if (bprm->envc < 0) {
        retval = bprm->envc;
        goto count_failed;
    }
    
    memset(bprm->buf, 0, BINPRM_BUF_SIZE);
    kernel_read(bprm->file, bprm->buf, BINPRM_BUF_SIZE);

    // 
    retval = prepare_elf_aux(bprm);
    if (retval < 0) {
        goto copy_failed;
    }

    retval = copy_strings(bprm->argc, argv, bprm);
    if (retval < 0) {
        goto copy_failed;
    }
    bprm->arg_start = bprm->p;

    retval = copy_strings(bprm->envc, envp, bprm);
    if (retval < 0) {
        goto copy_failed;
    }
    bprm->env_start = bprm->p;

    /*
     * 栈布局：
     *
     * argc
     * argv[]
     * NULL
     * envp[]
     * NULL
     * auxv[]
     * AT_NULL
     */
    retval = create_user_stack_layout(bprm->argc, argv, bprm->envc, envp, bprm);
    if (retval < 0) {
        goto create_failed;
    }

    retval = unshare_files_struct();
    if (retval < 0) {
        goto create_failed;
    }

    retval = search_binary_handler(bprm);
    if (retval < 0) {
        goto bin_failed;
    }

    do_close_on_exec(current->files);

    memset(current->comm, 0, sizeof(current->comm));
    strncpy(current->comm, exec_basename(filename), sizeof(current->comm) - 1);

    #ifdef SYS_TRACE_ENABLE
    printk("[exec] pid=%d exec %s ret=%ld\n",current->pid, filename, retval);
    #endif

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
    #ifdef SYS_TRACE_ENABLE
    printk(RED("[exec] pid=%d exec %s ret=%ld\n"),current->pid, filename, retval);
    #endif
    return retval;
}

int sys_execve(struct pt_regs *ctx) {
    const char *user_filename = (const char *)ctx->r[0];
    char *const *user_argv = (char *const *)ctx->r[1];
    char *const *user_envp = (char *const *)ctx->r[2];
    char *filename = NULL;
    char **argv = NULL;
    char **envp = NULL;
    int ret;

    ret = dup_user_string(user_filename, EXEC_PATH_MAX, &filename);
    if (ret < 0)
        return ret;

    ret = dup_user_argv(user_argv, &argv);
    if (ret < 0)
        goto out;

    ret = dup_user_argv(user_envp, &envp);
    if (ret < 0)
        goto out;

    ret = do_execve(filename, argv, envp);

out:
    free_kargv(envp);
    free_kargv(argv);
    kfree(filename);
    return ret;
}
