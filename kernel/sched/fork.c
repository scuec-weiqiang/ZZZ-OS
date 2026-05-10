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
    .flags = PF_KTHREAD,
    .prio		= 0,					

    .status = TASK_RUNNING,
    .stack = &init_thread_union,

    .wait = {
        .list = LIST_HEAD_INIT(init_task.wait.list),
        .private = &init_task,
    },
    .parent = &init_task,
    .children = LIST_HEAD_INIT(init_task.children),
    .sibling = LIST_HEAD_INIT(init_task.sibling),
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
    struct task_struct *task;
    list_for_each_entry(task, &global_rq->tasks, struct task_struct, task_node) {
        if (task->pid == pid) {
            return task;
        }
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

static pid_t alloc_pid(void) {
    static pid_t next_pid = 1;
    return next_pid++;
}

static void set_parent_child(struct task_struct *parent, struct task_struct *child) {
    child->parent = parent;
    list_add_tail(&parent->children, &child->sibling);
}

static void clear_parent_child(struct task_struct *parent, struct task_struct *child) {
    child->parent = NULL;
    list_del(&child->sibling);
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

    spin_lock_init(&tsk->lock);
    INIT_LIST_HEAD(&tsk->children);
    INIT_LIST_HEAD(&tsk->sibling);

    tsk->pid = alloc_pid();

    void *stack = alloc_stack();
    if (IS_ERR(stack)) {
        goto stack_failed;
    }

    tsk->stack = stack;

    // memcpy(tsk->stack, orig->stack, THREAD_SIZE);//复制内核栈内容
    setup_thread_stack(tsk, orig);

    return tsk;

stack_failed:
    free_task_struct(tsk);
task_failed:
    return ERR_PTR(-ENOMEM);
}

/* 目前实现为直接复制mm_struct */
static struct mm_struct *dup_mm(struct mm_struct *oldmm) {
    struct mm_struct *mm;
    struct vma *pos = NULL;
    int ret = -ENOMEM;

    mm = mm_alloc();
    if (!mm) {
        
        ret = -ENOMEM;
        goto fail;
    }
    
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
    list_for_each_entry(pos, &oldmm->vma_list.node, struct vma, node) {
        struct vma *new_vma = vma_clone(pos);
        virt_addr_t start;
        virt_addr_t end;
        pgprot_t flags;
        if (IS_ERR(new_vma)) {
            ret = PTR_ERR(new_vma);
            
            goto fail;
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
            goto fail;
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
                goto fail;
            }

            memcpy(newkva, (void *)KERNEL_VA(oldpa), PAGE_SIZE);
         
            ret = map(mm->pgdir, addr, KERNEL_PA(newkva), PAGE_SIZE, flags);
            if (ret < 0) {
                goto fail;
            }
        }
    }

    return mm;

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
    mm_destroy(task->mm);
    put_files_struct(task->files);
    put_fs_struct(task->fs);
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
    // 至此task结构体里除了文件描述符，内存管理等元数据外，调度相关的字段都设置好了

    struct files_struct *new_files = dup_fd(current->files);
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
    free_task_struct(p);
    return err;
}

pid_t do_fork_uthread(struct pt_regs *regs) {
    if (regs == NULL) {
        return -EINVAL;
    }

    int err = 0;

    struct task_struct *p = dup_task_struct(current);
    if (IS_ERR(p)) {
        
        return PTR_ERR(p);
    }


    sched_fork(p);
    // 至此task结构体里除了文件描述符，内存管理等元数据外，调度相关的字段都设置好了
    
    struct files_struct *new_files = dup_fd(current->files);
    if (IS_ERR(new_files)) {
        goto files_failed;
    }

    struct fs_struct *new_fs = copy_fs_struct(current->fs);
    if (IS_ERR(new_fs)) {
        
        goto fs_failed;
    }
    struct mm_struct *new_mm = dup_mm(current->mm);
    if (IS_ERR(new_mm)) {
        
        goto mm_failed;
    }
    p->fs = new_fs;
    p->files = new_files;
    p->active_mm = NULL;
    p->mm = new_mm;
    
    setup_uthread_context(p);

    set_parent_child(current, p);
    
    task_attach_to_rq(p);
    wake_up_process(p);
    
    return p->pid;

mm_failed:
    free_fs_struct(new_fs);
fs_failed:
    free_files_struct(new_files);
files_failed:
    free_task_struct(p);
    return err;
}

struct task_struct* setup_init_task(void) {
    struct task_struct *p = &init_task;
    p->sched_class = &rr_sched_class;
    p->se.exec_start = monotonic_ns();
    return &init_task;
}

void wake_up_process(struct task_struct *p) {
    struct rq *rq;
    struct thread_info *ti = task_thread_info(p);
    unsigned long flags;

    if (p == NULL || p == &init_task || p->status == TASK_RUNNING) {
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

    // if (current == rq->idle) {
    //     printk("waking up cpu %d for pid=%d\n", ti->cpu, p->pid);
    //     irq_send_ipi(ti->cpu, IPI_RESCHED);
    // }
}

int sys_fork(struct pt_regs *ctx) {
    return do_fork_uthread(ctx);
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

int kthreadd(void *arg) {
    int status = 0;
    while (1) {
        // 处理 kthread_create 请求
        struct kthread_create_info *info = NULL, *tmp  = NULL;
        pid_t pid = 0;
        if (!list_empty(&kthread_create_list)) {
            list_for_each_entry_safe(info,tmp,&kthread_create_list, struct kthread_create_info, list) {
                pid = kernel_thread(info->threadfn, info->data);
                if (IS_ERR((void*)pid)) {
                    panic("kthreadd: failed to create kernel thread for fn=%xu, data=%su, error=%d\n", info->threadfn, info->data, (int)pid);
                }
                info->result = find_task_by_pid(pid);
                dprintk("created kernel thread for fn=%xu, data=%su\n", info->threadfn, info->data);
                list_del(&info->list);
                complete(info->done);
            }
        }
        do_waitpid(-1, &status);
        sched();
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

    wake_up_process(kthreadd_task);
    wait_for_completion(&done);

    // dprintk(GREEN("pid=%d, threadfn=%xu, data=%xu, result=%xu\n"), info->result->pid, fn, arg, info->result);
    if (info->result == NULL) {
        return ERR_PTR(-EFAULT);
    }

    struct task_struct *result = info->result;
    kfree(info);

    return result;
}
