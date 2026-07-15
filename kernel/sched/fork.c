#include <os/sched.h>
#include <os/magic.h>
#include <os/check.h>
#include <os/compiler.h>
#include <os/cpu.h>
#include <os/irq.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/mm.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/string.h>
#include <fs/file.h>
#include <mm/pgtbl.h>
#include <asm/switch_to.h>
#include <asm/ptrace.h>
#include <mm/symbols.h>
#include <asm/atomic.h>
#include <fs/file.h>
#include <os/errno.h>
#include <fs/fs_struct.h>
#include <os/list.h>
#include <os/err.h>
#include <os/completion.h>
#include <mm/slab.h>
#include <asm/process.h>
#include <os/uaccess.h>

struct rq *global_rq __weak;

struct task_struct init_task;
extern void ret_from_fork(void);

__aligned(SIZE_8K) union thread_union init_thread_union = {
    .thread_info = {
        .task = &init_task,
        .cpu_context = {0},
        .cpu = 0,
    }
};

struct task_struct init_task = {
    .pid = 0,
    .tgid = 0,
    .comm = "swapper",
    .flags = PF_KTHREAD,
    .prio		= 1,
    .base_prio		= 1,

    .status = TASK_RUNNING,
    .stack = &init_thread_union,

    .wait = {
        .list = LIST_HEAD_INIT(init_task.wait.list),
        .private = &init_task,
    },
    .parent = &init_task,
    .group_leader = &init_task,
    .children = LIST_HEAD_INIT(init_task.children),
    .sibling = LIST_HEAD_INIT(init_task.sibling),
    .thread_group = LIST_HEAD_INIT(init_task.thread_group),
    .thread_node = LIST_HEAD_INIT(init_task.thread_node),
    .wait_child.head = LIST_HEAD_INIT(init_task.wait_child.head),

    .files = &init_files,
    .mm = NULL,
    .active_mm = &init_mm,
    .fs = &init_fs,

    .se= {
        .sched_node = LIST_HEAD_INIT(init_task.se.sched_node),
        .time_slice = NSEC_PER_SEC/100, 
    },
    .need_resched = 0,
    .sched_class = NULL,

    .task_node = LIST_HEAD_INIT(init_task.task_node),
};

struct task_struct *kthreadd_task;
struct kmem_cache *task_struct_cache;

struct kernel_clone_args {
    unsigned long flags;
    unsigned long stack;
    int __user *parent_tid;
    int __user *child_tid;
    unsigned long tls;
    int exit_signal;
};

static void *alloc_stack(void) {
    void *stack = kmalloc(THREAD_SIZE);
    if (!stack) {
        return ERR_PTR(-ENOMEM);
    }
    memset(stack, 0, THREAD_SIZE);
    return stack;
}

static void free_stack(struct task_struct *task) {
    if (task->stack) {
        kfree(task->stack);
    }
}

struct task_struct* find_task_by_pid(pid_t pid) {
    int cpu_num;

    if (global_rq == NULL) {
        return NULL;
    }

    cpu_num = smp_get_cpu_count();
    if (cpu_num > MAX_CPUS) {
        cpu_num = MAX_CPUS;
    }

    for (int cpu = 0; cpu < cpu_num; cpu++) {
        struct rq *rq = &global_rq[cpu];
        struct task_struct *task;
        unsigned long flags;

        flags = spin_lock_irqsave(&rq->lock);
        list_for_each_entry(task, &rq->tasks, task_node) {
            if (task->pid == pid) {
                spin_unlock_irqrestore(&rq->lock, flags);
                return task;
            }
        }
        spin_unlock_irqrestore(&rq->lock, flags);
    }

    return NULL;
}

int alloc_task_struct_init(void) {
    task_struct_cache = kmem_cache_create("task_struct_cache", sizeof(struct task_struct), 8);
    if (!task_struct_cache) {
        return -ENOMEM;
    }
    return 0;
}

static struct task_struct *alloc_task_struct(void) {
    struct task_struct *task  = kmem_cache_alloc(task_struct_cache);
    if (!task) {
        return ERR_PTR(-ENOMEM);
    }
    memset(task, 0, sizeof(struct task_struct));

    task->status = TASK_SLEEPING;
    task->wait_reason = get_wait_reason_name(WAIT_NONE);
    INIT_LIST_HEAD(&task->task_node);
    INIT_LIST_HEAD(&task->se.sched_node);
    INIT_LIST_HEAD(&task->children);
    INIT_LIST_HEAD(&task->sibling);
    INIT_LIST_HEAD(&task->wait.list);
    INIT_LIST_HEAD(&task->wait_child.head);
    task->wait.private = task;

    return task;
}

static void free_task_struct(struct task_struct *obj) {
    if (!obj) return;
    kmem_cache_free(obj);
}

static int put_user_int_to_mm(struct mm_struct *mm, int __user *uaddr, int val)
{
    unsigned char *src = (unsigned char *)&val;

    if (uaddr == NULL) {
        return -EFAULT;
    }
    if (mm == NULL || mm->pgdir == NULL) {
        return -EFAULT;
    }

    for (size_t i = 0; i < sizeof(val); i++) {
        virt_addr_t va = (virt_addr_t)uaddr + i;
        phys_addr_t pa = pgtbl_lookup(mm->pgdir, va);

        if (pa == 0) {
            return -EFAULT;
        }
        *(unsigned char *)KERNEL_VA(pa) = src[i];
    }

    return 0;
}

void task_set_mm(struct task_struct *task, struct mm_struct *mm)
{
    struct mm_struct *old_mm;
    struct mm_struct *old_active_mm;

    if (task == NULL) {
        return;
    }

    old_mm = task->mm;
    old_active_mm = task->active_mm;
    task->mm = mm;
    task->active_mm = mm;

    if (old_mm != NULL && old_mm != mm) {
        mmput(old_mm);
    }
    if (old_active_mm != NULL && old_active_mm != old_mm &&
        old_active_mm != mm && old_active_mm != &init_mm) {
        mmput(old_active_mm);
    }
}

void task_drop_mm(struct task_struct *task)
{
    struct mm_struct *mm;
    struct mm_struct *active_mm;

    if (task == NULL) {
        return;
    }

    mm = task->mm;
    active_mm = task->active_mm;
    task->mm = NULL;
    task->active_mm = NULL;

    if (mm != NULL) {
        mmput(mm);
    }
    if (active_mm != NULL && active_mm != mm && active_mm != &init_mm) {
        mmput(active_mm);
    }
}

static pid_t alloc_pid(void) {
    static pid_t next_pid = 1;
    return next_pid++;
}

static struct signal_struct *copy_signal_struct(struct task_struct *orig,
                                                struct task_struct *tsk) {
    struct signal_struct *sig;

    sig = kmalloc(sizeof(*sig));
    if (sig == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    if (orig != NULL && orig->signal != NULL &&
        orig->signal->pgrp > 0 && orig->signal->session > 0) {
        *sig = *orig->signal;
    } else {
        memset(sig, 0, sizeof(*sig));
        sig->pgrp = tsk->pid;
        sig->session = tsk->pid;
    }

    return sig;
}

static void free_signal_struct(struct signal_struct *sig) {
    if (sig != NULL) {
        kfree(sig);
    }
}

static void set_parent_child(struct task_struct *parent, struct task_struct *child) {
    unsigned long flags;

    flags = spin_lock_irqsave(&parent->lock);
    child->parent = parent;
    child->ppid = parent->pid;
    list_add_tail(&parent->children, &child->sibling);
    spin_unlock_irqrestore(&parent->lock, flags);
}

static void clear_parent_child(struct task_struct *parent, struct task_struct *child) {
    unsigned long flags;

    if (parent == NULL) {
        return;
    }

    flags = spin_lock_irqsave(&parent->lock);
    child->parent = NULL;
    list_del(&child->sibling);
    spin_unlock_irqrestore(&parent->lock, flags);
}

/*
    复制原进程的文件描述符、内存、文件系统等元数据，分配新pid，设置新父子状态
    但不复制运行时的队列状态
*/
static struct task_struct *dup_task_struct(struct task_struct *orig) {
    struct task_struct *tsk;

    // 分配task内存
    tsk = alloc_task_struct();
    if (IS_ERR(tsk)) goto task_failed;
    
    *tsk = *orig; // 复制task_struct的内容

    tsk->status = TASK_SLEEPING;
    tsk->sched_class = &rr_sched_class;
    tsk->mm = NULL;
    tsk->active_mm = NULL;
    tsk->vfork_done = NULL;
    tsk->set_child_tid = NULL;
    tsk->clear_child_tid = NULL;

    spin_lock_init(&tsk->lock);
    INIT_LIST_HEAD(&tsk->children);
    INIT_LIST_HEAD(&tsk->sibling);
    INIT_LIST_HEAD(&tsk->thread_group);
    INIT_LIST_HEAD(&tsk->thread_node);

    tsk->pid = alloc_pid();
    tsk->tgid = tsk->pid;
    tsk->group_leader = tsk;
    tsk->signal = copy_signal_struct(orig, tsk);
    if (IS_ERR(tsk->signal)) {
        goto signal_failed;
    }
    tsk->wait_child.wait_reason = get_wait_reason_name(WAIT_CHILD);
    void *stack = alloc_stack();
    if (IS_ERR(stack)) {
        goto stack_failed;
    }

    tsk->stack = stack;

    // memcpy(tsk->stack, orig->stack, THREAD_SIZE);//复制内核栈内容
    setup_thread_stack(tsk, orig);

    return tsk;

stack_failed:
    free_signal_struct(tsk->signal);
signal_failed:
    free_task_struct(tsk);
task_failed:
    return ERR_PTR(-ENOMEM);
}

/* 目前实现为直接复制mm_struct */
static struct mm_struct *dup_mm(struct mm_struct *oldmm, unsigned long flags) {
    struct mm_struct *mm;
    struct vma *pos = NULL;
    unsigned long lock_flags;
    int ret = -ENOMEM;

    if (oldmm == NULL) {
        return ERR_PTR(-EINVAL);
    }

    if (flags & CLONE_VM) {
        mmget(oldmm);
        return oldmm;
    }

    mm = mm_alloc();
    if (!mm) {
        
        ret = -ENOMEM;
        goto fail;
    }

    lock_flags = spin_lock_irqsave(&oldmm->lock);
    
    mm->start_stack = oldmm->start_stack;
    mm->stack_top = oldmm->stack_top;
    mm->stack_prot = oldmm->stack_prot;
    mm->start_code = oldmm->start_code;
    mm->end_code = oldmm->end_code;
    mm->start_data = oldmm->start_data;
    mm->end_data = oldmm->end_data;
    mm->start_brk = oldmm->start_brk;
    mm->brk = oldmm->brk;
    mm->vma_count = oldmm->vma_count;

    copy_kernel_mapping(mm);
    // 
    list_for_each_entry(pos, &oldmm->vma_list.node, node) {
        struct vma *new_vma = vma_clone(pos);
        virt_addr_t start;
        virt_addr_t end;
        pgprot_t flags;
        if (IS_ERR(new_vma)) {
            ret = PTR_ERR(new_vma);
            
            goto fail_unlock;
        }

        /*
         * vma_insert() may merge and free @new_vma, so cache the
         * range/flags before insertion and never dereference it again
         * afterwards.
         */
        start = new_vma->start;
        end = new_vma->end;
        flags = new_vma->flags;

        ret = vma_insert(mm, new_vma);
        if (ret < 0) {
            
            vma_destroy(new_vma);
            goto fail_unlock;
        }

        for (virt_addr_t addr = ALIGN_DOWN(start, PAGE_SIZE); addr < end; addr += PAGE_SIZE) {
            phys_addr_t oldpa = pgtbl_lookup(oldmm->pgdir, addr);
            void *newkva;

            if (!oldpa) {
                
                continue;
            }

            newkva = page_alloc(1);
            if (!newkva) {
                ret = -ENOMEM;
                goto fail_unlock;
            }

            memcpy(newkva, (void *)KERNEL_VA(oldpa), PAGE_SIZE);
         
            ret = map(mm->pgdir, addr, KERNEL_PA(newkva), PAGE_SIZE, flags);
            if (ret < 0) {
                goto fail_unlock;
            }
        }
    }

    spin_unlock_irqrestore(&oldmm->lock, lock_flags);
    return mm;

fail_unlock:
    spin_unlock_irqrestore(&oldmm->lock, lock_flags);
fail:
    mm_destroy(mm);
    return ERR_PTR(ret);
}

/* 请确保此时task已不在运行队列中 */
void task_destroy(struct task_struct *task) {
    if (task == NULL || task == &init_task) {
        return;
    }
    free_stack(task);
    clear_parent_child(task->parent, task);
    task_drop_mm(task);
    put_files_struct(task->files);
    put_fs_struct(task->fs);
    free_signal_struct(task->signal);
    task->signal = NULL;
    free_task_struct(task);
}

pid_t do_fork_kthread(int (*fn)(void *), void *arg) {
    if (fn == NULL) {
        return -EINVAL;
    }

    int err = 0;

    struct task_struct *p = dup_task_struct(current);
    if (IS_ERR(p)) {
        return PTR_ERR(p);
    }

    sched_fork(p);
    strncpy(p->comm, "kthread", sizeof(p->comm) - 1);
    // 至此task结构体里除了文件描述符，内存管理等元数据外，调度相关的字段都设置好了

    struct files_struct *new_files = dup_fd(current->files, 0);
    if (IS_ERR(new_files)) {
        goto files_failed;
    }
    p->files = new_files;

    // 内核线程共享父进程的fs_struct
    p->fs = get_fs_struct(current->fs);
    setup_kthread_context(fn, arg, p);

    p->active_mm = NULL;
    p->mm = NULL;

    set_parent_child(current, p);

    // 加入运行队列
    task_attach_to_rq(p);
    wake_up_process(p);

    return p->pid;

files_failed:
    free_signal_struct(p->signal);
    free_stack(p);
    free_task_struct(p);
    return err;
}

pid_t kernel_thread_on_cpu(int (*fn)(void *), void *arg, int cpu)
{
    int err = 0;
    struct files_struct *new_files;
    struct task_struct *p;

    if (fn == NULL) {
        return -EINVAL;
    }

    p = dup_task_struct(current);
    if (IS_ERR(p)) {
        return PTR_ERR(p);
    }

    sched_fork(p);
    err = task_bind_cpu(p, cpu);
    if (err < 0) {
        goto bind_failed;
    }

    new_files = dup_fd(current->files, 0);
    if (IS_ERR(new_files)) {
        err = PTR_ERR(new_files);
        goto files_failed;
    }
    p->files = new_files;

    p->fs = get_fs_struct(current->fs);
    setup_kthread_context(fn, arg, p);

    p->active_mm = NULL;
    p->mm = NULL;

    set_parent_child(current, p);

    task_attach_to_rq(p);
    wake_up_process(p);

    return p->pid;

files_failed:
bind_failed:
    free_signal_struct(p->signal);
    free_stack(p);
    free_task_struct(p);
    return err;
}

pid_t do_fork_uthread(struct pt_regs *regs, const struct kernel_clone_args *args) {
    struct completion vfork;
  
    unsigned long clone_flags = args ? args->flags : SIGCHLD;

    if (regs == NULL) {
        return -EINVAL;
    }

    int do_vfork = clone_flags & CLONE_VFORK;
    if (do_vfork) {
        init_completion(&vfork);
    }

    int err = 0;

    struct task_struct *p = dup_task_struct(current);
    if (IS_ERR(p)) {
        
        return PTR_ERR(p);
    }

    sched_fork(p);
    // 至此task结构体里除了文件描述符，内存管理等元数据外，调度相关的字段都设置好了
    
    struct files_struct *new_files = NULL;
    struct fs_struct *new_fs = NULL;
    struct mm_struct *new_mm = NULL;

    new_files = dup_fd(current->files, clone_flags);
    if (IS_ERR(new_files)) {
        err = PTR_ERR(new_files);
        goto files_failed;
    }

    new_fs = copy_fs_struct(current->fs, clone_flags);
    if (IS_ERR(new_fs)) {
        err = PTR_ERR(new_fs);
        goto fs_failed;
    }
    
    new_mm = dup_mm(current->mm, clone_flags);
    if (IS_ERR(new_mm)) {
        err = PTR_ERR(new_mm);
        goto mm_failed;
    }

    p->fs = new_fs;
    p->files = new_files;
    p->active_mm = NULL;
    p->mm = new_mm;
    p->set_child_tid = (args && (clone_flags & CLONE_CHILD_SETTID)) ?
        args->child_tid : NULL;
    p->clear_child_tid = (args && (clone_flags & CLONE_CHILD_CLEARTID)) ?
        args->child_tid : NULL;
    p->vfork_done = do_vfork ? &vfork : NULL;
    
    setup_uthread_context(p, args ? args->stack : 0,
                          args ? args->tls : 0, clone_flags);

    if (clone_flags & CLONE_CHILD_SETTID) {
        err = put_user_int_to_mm(p->mm, p->set_child_tid, p->pid);
        if (err < 0) {
            goto child_tid_failed;
        }
    }

    set_parent_child(current, p);

    if (clone_flags & CLONE_PARENT_SETTID) {
        err = copy_to_user((char *)args->parent_tid, (char *)&p->pid,
                           sizeof(p->pid));
        if (err < 0) {
            goto parent_tid_failed;
        }
    }

    task_attach_to_rq(p);
    wake_up_process(p);

    if (do_vfork) {
        wait_for_completion(&vfork);
    }
    
    return p->pid;

parent_tid_failed:
    clear_parent_child(current, p);
child_tid_failed:
    mm_destroy(new_mm);
mm_failed:
    put_fs_struct(new_fs);
fs_failed:
    put_files_struct(new_files);
files_failed:
    free_signal_struct(p->signal);
    free_stack(p);
    free_task_struct(p);
    return err;
}

struct task_struct* setup_init_task(void) {
    struct task_struct *p = &init_task;
    if (p->signal == NULL) {
        p->signal = kmalloc(sizeof(*p->signal));
        if (p->signal == NULL) {
            panic("failed to allocate init signal_struct");
        }
        memset(p->signal, 0, sizeof(*p->signal));
        p->signal->pgrp = p->pid;
        p->signal->session = p->pid;
    }
    p->sched_class = &rr_sched_class;
    p->se.exec_start = monotonic_ns();
    p->on_cpu = 1;
    return &init_task;
}

void wake_up_process(struct task_struct *p) {
    struct rq *rq;
    struct thread_info *ti = task_thread_info(p);
    unsigned long flags;

    if (p == NULL || p == &init_task || p->status != TASK_SLEEPING) {
        return;
    }

    p->status = TASK_RUNNING;
    if (global_rq == NULL || p->sched_class == NULL || p->sched_class->enqueue_task == NULL) {
        return;
    }

    rq = &global_rq[ti->cpu];
    flags = spin_lock_irqsave(&rq->lock);
    p->sched_class->enqueue_task(rq, p);
    spin_unlock_irqrestore(&rq->lock, flags);

    sched_resched_cpu(ti->cpu);
}

int wake_up_process_on(struct task_struct *p, int cpu)
{
    int ret;

    ret = task_bind_cpu(p, cpu);
    if (ret < 0) {
        return ret;
    }

    wake_up_process(p);
    return 0;
}

__SYSCALL__ long sys_clone(struct pt_regs *ctx)
{
    unsigned long flags = ctx->r[0];
    unsigned long new_stack = ctx->r[1];
    unsigned long parent_tid = ctx->r[2];
    unsigned long tls = ctx->r[3];
    unsigned long child_tid = ctx->r[4];
    struct kernel_clone_args args;
    unsigned long supported;

    supported = CSIGNAL | CLONE_FS | CLONE_FILES | CLONE_SETTLS | CLONE_PARENT_SETTID |
                CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID | CLONE_VFORK | CLONE_VM;
    if ((flags & ~supported) != 0) {
        return -EINVAL;
    }

    memset(&args, 0, sizeof(args));
    args.flags = flags;
    args.stack = new_stack;
    args.parent_tid = (int __user *)parent_tid;
    args.child_tid = (int __user *)child_tid;
    args.tls = tls;
    args.exit_signal = flags & CSIGNAL;

    if ((flags & CLONE_PARENT_SETTID) && args.parent_tid == NULL) {
        return -EINVAL;
    }
    if ((flags & (CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID)) &&
        args.child_tid == NULL) {
        return -EINVAL;
    }

    return do_fork_uthread(ctx, &args);
}

__SYSCALL__ long sys_fork(struct pt_regs *ctx) {
    struct kernel_clone_args args;

    memset(&args, 0, sizeof(args));
    args.flags = SIGCHLD;
    args.exit_signal = SIGCHLD;

    return do_fork_uthread(ctx, &args);
}

/*
 * Create a kernel thread.
 */
pid_t kernel_thread(int (*fn)(void *), void *arg) {
	return do_fork_kthread(fn,arg);
}

struct kthread_create_info {
	/* Information passed to kthread() from kthreadd. */
	int (*threadfn)(void *data);
	void *data;

	/* Result passed back to kthread_create() from kthreadd. */
	struct task_struct *result;
    struct completion *done;
	struct list_head list;
};

// 需要创建的内核进程挂在这个链表下，等待kthreadd来处理
static struct list_head kthread_create_list = LIST_HEAD_INIT(kthread_create_list);
static struct wait_queue_head kthread_create_wait = WAIT_QUEUE_INIT(kthread_create_wait);

int kthreadd(void *arg) {
    int status = 0;
    kthread_create_wait.wait_reason = get_wait_reason_name(WAIT_IDLE);
    while (1) {
        // 处理 kthread_create 请求
        struct kthread_create_info *info = NULL, *tmp  = NULL;
        pid_t pid = 0;

        while (do_waitpid(-1, &status, WNOHANG) > 0) {
        }

        if (list_empty(&kthread_create_list)) {
            sleep_on(&kthread_create_wait);
            continue;
        }

        if (!list_empty(&kthread_create_list)) {
            list_for_each_entry_safe(info, tmp, &kthread_create_list, list) {
                pid = kernel_thread(info->threadfn, info->data);
                if (pid < 0) {
                    panic("kthreadd: failed to create kernel thread for fn=%xu, data=%su, error=%d\n", info->threadfn, info->data, (int)pid);
                }
                info->result = find_task_by_pid(pid);
                dprintk("created kernel thread for fn=%xu, data=%su\n", info->threadfn, info->data);
                list_del(&info->list);
                complete(info->done);
            }
        }
    }
}

struct task_struct* kthread_create(int (*fn)(void *), void *arg) {
    if (fn == NULL) {
        return ERR_PTR(-EINVAL);
    }

    struct kthread_create_info *info;
    struct completion done = {
        .done = 0,
        .wait = {
            .head = LIST_HEAD_INIT(done.wait.head),
        },
    };

    info = kmalloc(sizeof(*info));
    
    if (!info) {
        return ERR_PTR(-ENOMEM);
    }

    info->threadfn = fn;
    info->data = arg;
    info->done = &done;
    INIT_LIST_HEAD(&info->list);
    list_add_tail(&kthread_create_list,&info->list);

    wake_up_one(&kthread_create_wait);
    wait_for_completion(&done);

    // dprintk(GREEN("pid=%d, threadfn=%xu, data=%xu, result=%xu\n"), info->result->pid, fn, arg, info->result);
    if (info->result == NULL) {
        return ERR_PTR(-EFAULT);
    }

    struct task_struct *result = info->result;
    kfree(info);

    return result;
}
