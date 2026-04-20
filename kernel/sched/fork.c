#include <os/sched.h>
#include <os/magic.h>
#include <os/check.h>
#include <os/compiler_attributes.h>
#include <os/cpu.h>
#include <os/elf.h>
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
#include <os/errno-base.h>
#include <fs/fs_struct.h>
#include <os/list.h>
#include <os/err.h>
#include <os/completion.h>
#include <asm/process.h>

struct rq *global_rq __weak;

struct task_struct init_task;
extern void ret_from_fork(void);

__aligned(SIZE_8K) union thread_union init_thread_union = {
    .thread_info = {
        .task = &init_task,
        .cpu_context = {0},
        .preempt_count = 0,
        .cpu = 0,
        .syscall = 0,
    }
};

struct task_struct init_task = {
    .pid = 0,
    .tgid = 0,
    .flags = PF_KTHREAD,

    .prio		= 0,					
	.static_prio	= 0,					
	.normal_prio	= 0,		

    .status = TASK_RUNNING,
    .stack = &init_thread_union,

    .parent = &init_task,
    .children = LIST_HEAD_INIT(init_task.children),
    .sibling = LIST_HEAD_INIT(init_task.sibling),

    .files = &init_files,
    .mm = NULL,
    .active_mm = &init_mm,
    .fs = &init_fs,

    .se= {
        .sched_node = LIST_HEAD_INIT(init_task.se.sched_node),
        .time_slice = NSEC_PER_SEC, 
    },
    .need_resched = 0,
    .sched_class = NULL,

    .task_node = LIST_HEAD_INIT(init_task.task_node),
    

};

struct task_struct *kthreadd_task;

static pid_t alloc_pid(void) {
    static pid_t next_pid = 1;
    return next_pid++;
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

void set_task_stack_end_magic(struct task_struct *tsk) {
	unsigned long *stackend;

	stackend = end_of_stack(tsk);
	*stackend = STACK_END_MAGIC;	/* for overflow detection */
}

/*
do_fork 
    copy_process
        new_task = dup_task_struct(parent) 
            分配thread_info 内存，从旧进程复制thread_info
            设置栈magic
        sched_fork 新进程初始化调度器相关内容
        copy_files
        copy_fs
        copy_mm
        copy_thread 
            设置新进程的thread info，准备上下文
                if 内核线程
                    r4 = stack_sz(内核线程参数)
                    r5 = stack_start(内核线程函数地址)
                    pt_regs cpsr = svc_mode
                else 用户进程
                    复制父进程pt_regs
                    r0 = 0
                    sp = stack_start
        alloc_pid
        设置父子进程
*/
static int dup_mmap(struct mm_struct *mm, struct mm_struct *oldm) {
    struct vma *vma = &mm->vma_list, *old_vma;
    int ret = 0;

    /* Copy each VMA from oldmm to newmm */
    list_for_each_entry(old_vma, &oldm->vma_list.node, struct vma, node) {
        vma = vma_clone(old_vma);
        if (!vma) {
            ret = -ENOMEM;
            break;
        }
        list_add(&mm->vma_list.node,&vma->node);
    }

    return ret;
}

static struct mm_struct *dup_mm(struct task_struct *tsk) {
	struct mm_struct *mm, *oldmm = current->mm;
	int err;

	mm = mm_alloc();
	if (!mm)
		goto fail_nomem;

	memcpy(mm, oldmm, sizeof(*mm));

    *(mm->pgdir) = *(oldmm->pgdir);
    mm->start_stack = oldmm->start_stack;
    mm->start_code = oldmm->start_code;
    mm->end_code = oldmm->end_code;
    mm->start_data = oldmm->start_data;
    mm->end_data = oldmm->end_data;
    mm->start_brk = oldmm->start_brk;
    mm->brk = oldmm->brk;

	err = dup_mmap(mm, oldmm);
	if (err)
		goto fail_nomem;
    
	return mm;

fail_nomem:
	return NULL;
}

static int copy_mm(unsigned long clone_flags, struct task_struct *tsk) {
	struct mm_struct *mm, *oldmm;
	int retval;

	tsk->mm = NULL;
	tsk->active_mm = NULL;

	/*
	 * Are we cloning a kernel thread?
	 *
	 * We need to steal a active VM for that..
	 */
	oldmm = current->mm;
	if (!oldmm)
		return 0;

	if (clone_flags & CLONE_VM) {
		mm = oldmm;
		goto good_mm;
	}

	retval = -ENOMEM;
	mm = dup_mm(tsk);
	if (!mm)
		goto fail_nomem;

good_mm:
	tsk->mm = mm;
	tsk->active_mm = mm;
	return 0;

fail_nomem:
	return retval;
}

static int copy_fs(unsigned long clone_flags, struct task_struct *tsk) {
	struct fs_struct *fs = current->fs;
	if (clone_flags & CLONE_FS) {
		/* tsk->fs is already what we want */
		spin_lock(&fs->lock);
		if (fs->in_exec) {
			spin_unlock(&fs->lock);
			return -EAGAIN;
		}
		fs->users++;
		spin_unlock(&fs->lock);
		return 0;
	}
	tsk->fs = copy_fs_struct(fs);
	if (!tsk->fs)
		return -ENOMEM;
	return 0;
}

static int copy_files(unsigned long clone_flags, struct task_struct *tsk) {
	struct files_struct *oldf, *newf;
	int error = 0;

	/*
	 * A background process may not have any files ...
	 */
	oldf = current->files;
	if (!oldf)
		goto out;

	if (clone_flags & CLONE_FILES) {
		atomic_inc(&oldf->refcount);
		goto out;
	}

	newf = dup_fd(oldf, &error);
	if (!newf)
		goto out;

	tsk->files = newf;
	error = 0;
out:
	return error;
}

static struct task_struct *alloc_task_struct(void) {
    struct task_struct *task = (struct task_struct *)kmalloc(sizeof(*task));

    CHECK(task != NULL, "task alloc failed", return NULL;);
    memset(task, 0, sizeof(*task));

    task->status = TASK_WAITING;
    task->se.time_slice = PROC_DEFAULT_SLICE;
    task->sched_class = &rr_sched_class;
    INIT_LIST_HEAD(&task->children);
    INIT_LIST_HEAD(&task->sibling);
    INIT_LIST_HEAD(&task->task_node);
    INIT_LIST_HEAD(&task->se.sched_node);
    return task;
}

static void *alloc_task_stack(void) {
    void *stack = kmalloc(THREAD_SIZE);
    CHECK(stack != NULL, "task stack alloc failed", return NULL;);
    memset(stack, 0, THREAD_SIZE);
    return stack;
}


/*复制task元数据骨架，但不复制运行时的队列状态*/
static struct task_struct *dup_task_struct(struct task_struct *orig) {
    struct task_struct *tsk;
    struct thread_info *ti;
    
    // 分配task内存
    tsk = alloc_task_struct();
    if (!tsk) return NULL;
    *tsk = *orig; // 复制task_struct的内容

    // 分配内核栈的内存
    ti = alloc_task_stack();
    tsk->stack = ti;

    setup_thread_stack(tsk, orig);//复制内核栈

    set_task_stack_end_magic(tsk);

    return tsk;
}

static struct task_struct *copy_process(unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size) {
    int retval = 0;
    struct task_struct *p;

    p = dup_task_struct(current);
    if (!p) {
        retval = -ENOMEM;
        goto out;
    }

    sched_fork(p);
    // copy_files(clone_flags, p);
    copy_fs(clone_flags, p);
    copy_mm(clone_flags, p);
    copy_thread(clone_flags, stack_start, stack_size, p);

    return p;
out:
    return (void *)retval;
}

void task_destroy(struct task_struct *task) {
    if (task == NULL || task == &init_task) {
        return;
    }
    kfree(task->stack);
    kfree(task->mm);
    // kfree(task->files);

    if (task->fs->users != 1) {
        spin_lock(&task->fs->lock);
        if (task->fs->users > 0) {
            task->fs->users--;
        }
        spin_unlock(&task->fs->lock);
    } else {
        kfree(task->fs);
    }

    kfree(task);
    
}

void wake_up_process(struct task_struct *p) {
    struct rq *rq;
    struct thread_info *ti = task_thread_info(p);
    unsigned long flags;

    CHECK(p != NULL, "task wakeup: task is NULL", return;);

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

struct task_struct* setup_init_task(void) {
    struct task_struct *p = &init_task;
    p->sched_class = &rr_sched_class;
    return &init_task;
}

long do_fork(unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size) {
	struct task_struct *p;
	long nr;

	p = copy_process(clone_flags, stack_start, stack_size);
    p->pid = alloc_pid();
    p->tgid = p->pid;

	if (!IS_ERR(p)) {
		wake_up_process(p);
        nr = p->pid;
	} else {
		nr = PTR_ERR(p);
	}

	return nr;
}

/*
 * Create a kernel thread.
 */
pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags) {
	return do_fork(flags|CLONE_VM, (unsigned long)fn,
		(unsigned long)arg);
}

struct kthread_create_info
{
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
    while (1) {
        // 处理 kthread_create 请求
        struct kthread_create_info *info = NULL, *tmp  = NULL;
        pid_t pid = 0;
        if (!list_empty(&kthread_create_list)) {
            list_for_each_entry_safe(info,tmp,&kthread_create_list, struct kthread_create_info, list) {
                pid = kernel_thread(info->threadfn, info->data, CLONE_FS);
                if (IS_ERR((void*)pid)) {
                    panic("kthreadd: failed to create kernel thread for fn=%xu, data=%su, error=%d\n", info->threadfn, info->data, (int)pid);
                }
                info->result = find_task_by_pid(pid);
                dprintk("created kernel thread for fn=%xu, data=%su\n", info->threadfn, info->data);
                list_del(&info->list);
                complete(info->done);
            }
        }
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
    list_add_tail(&info->list, &kthread_create_list);

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

void dump_task(struct task_struct *task) {
    if (task == NULL) {
        return;
    }
    printk("task dump: pid=%d, tgid=%d, status=%d, flags=%x\n", task->pid, task->tgid, task->status, task->flags);
    printk("  mm: start_code=0x%lx, end_code=0x%lx, start_data=0x%lx, end_data=0x%lx, start_stack=0x%lx\n",
           task->mm ? task->mm->start_code : 0,
           task->mm ? task->mm->end_code : 0,
           task->mm ? task->mm->start_data : 0,
           task->mm ? task->mm->end_data : 0,
           task->mm ? task->mm->start_stack : 0);
    printk("stack dump:\n");
    if (task->stack) {
        struct thread_info *ti = task_thread_info(task);
        printk("  thread_info: cpu=%d, preempt_count=%d, syscall=%d\n", ti->cpu, ti->preempt_count, ti->syscall);
        printk(" cpu_context: r4=%xu, r5=%xu, r6=%xu, r7=%xu, r8=%xu, r9=%xu, sl=%xu, fp=%xu\n",
               ti->cpu_context.r4, ti->cpu_context.r5, ti->cpu_context.r6, ti->cpu_context.r7,
               ti->cpu_context.r8, ti->cpu_context.r9, ti->cpu_context.sl, ti->cpu_context.fp);
        printk("  sp=%xu, pc=%xu\n",
               ti->cpu_context.sp, ti->cpu_context.pc);
    }
}
  
/*------------------------------------------------------------------*/
// void __weak __noreturn arch_user_enter(struct pt_regs *tf) {
//     (void)tf;
//     panic("arch_user_enter is not implemented for this architecture\n");
//     for (;;) {
//         cpu_idle();
//     }
// }

// static pgprot_t task_segment_pgprot(uint32_t elf_flags) {
//     pgprot_t prot = PROT_USER;

//     if (elf_flags & PF_R) {
//         prot |= PROT_READ;
//     }
//     if (elf_flags & PF_W) {
//         prot |= PROT_WRITE;
//     }
//     if (elf_flags & PF_X) {
//         prot |= PROT_EXEC;
//     }
//     if ((prot & (PROT_READ | PROT_WRITE | PROT_EXEC)) == 0) {
//         prot |= PROT_READ;
//     }
//     return prot;
// }

// static int task_load_elf_segments(struct task_struct *task) {
//     struct mm_struct *mm = task->mm;

//     CHECK(task->elf_info != NULL, "task load: missing elf info", return -1;);
//     CHECK(mm != NULL, "task load: missing mm", return -1;);

//     for (int i = 0; i < task->elf_info->phnum; i++) {
//         struct elf_segment *seg = &task->elf_info->segs[i];
//         pgprot_t prot;

//         if (seg->type != PT_LOAD || seg->memsz == 0) {
//             continue;
//         }

//         CHECK(seg->offset + seg->filesz <= task->elf_size, "task load: segment out of range", return -1;);

//         prot = task_segment_pgprot(seg->flags);
//         CHECK(task_map_user_range(mm,
//                                   (virt_addr_t)seg->vaddr,
//                                   task->elf_image + seg->offset,
//                                   (size_t)seg->filesz,
//                                   (size_t)seg->memsz,
//                                   prot,
//                                   &task->segment_backing[i]) == 0,
//               "task load: map segment failed", return -1;);

//         if (task->mm->start_code == 0 || seg->vaddr < task->mm->start_code) {
//             task->mm->start_code = seg->vaddr;
//         }
//         if (task->mm->end_code < seg->vaddr + seg->memsz) {
//             task->mm->end_code = seg->vaddr + seg->memsz;
//         }
//         if (seg->flags & PF_W) {
//             if (task->mm->start_data == 0 || seg->vaddr < task->mm->start_data) {
//                 task->mm->start_data = seg->vaddr;
//             }
//             if (task->mm->end_data < seg->vaddr + seg->memsz) {
//                 task->mm->end_data = seg->vaddr + seg->memsz;
//             }
//         }
//     }

//     return 0;
// }

// static int task_prepare_user(struct task_struct *task) {
//     task->mode = TASK_MODE_USER;
//     task->mm = mm_alloc("user_pgtbl");
//     CHECK(task->mm != NULL, "task user: create mm failed", return -1;);

//     copy_kernel_mapping(task->mm);

//     task->ustack = alloc_task_stack();
//     CHECK(task->ustack != NULL, "task user: alloc user stack failed", return -1;);
//     task->ustack_top = (void *)PROC_USER_STACK_BOTTOM;
//     task->mm->start_stack = (unsigned long)task->ustack_top;

//     CHECK(do_mmap(task->mm, PROC_USER_STACK_TOP, PROC_STACK_SIZE, PROT_USER | PROT_READ | PROT_WRITE) == 0,
//           "task user: add user stack vma failed", return -1;);
//     CHECK(map(task->mm->pgdir,
//               PROC_USER_STACK_TOP,
//               KERNEL_PA(task->ustack),
//               PROC_STACK_SIZE,
//               PROT_USER | PROT_READ | PROT_WRITE) == 0,
//           "task user: map user stack failed", return -1;);

//     CHECK(task_load_elf_segments(task) == 0, "task user: load elf failed", return -1;);

//     task->pt_regs = (struct pt_regs *)((uintptr_t)task->kstack_top - sizeof(struct pt_regs));
//     trapframe_setup_user(task->pt_regs, (reg_t)task->elf_info->entry, (reg_t)task->ustack_top, 0);

//     ctx_init(&task->ctx, (reg_t)task_user_bootstrap, (reg_t)task->pt_regs);
//     return 0;
// }

// struct task_struct *task_create_user(const char *path) {
//     struct task_struct *task;
//     struct file *file;
//     ssize_t read_len;

//     CHECK(path != NULL, "task create: path is NULL", return NULL;);

//     file = filp_open(path, 0);
//     CHECK(file != NULL, "task create: open elf failed", return NULL;);
//     CHECK(file->f_inode != NULL, "task create: invalid elf inode", filp_close(file); return NULL;);

//     task = alloc_task_struct();
//     CHECK(task != NULL, "task create: alloc user task failed", filp_close(file); return NULL;);

//     task->kstack = alloc_task_stack();
//     CHECK(task->kstack != NULL, "task create: alloc kernel stack failed", filp_close(file); kfree(task); return NULL;);
//     task->kstack_top = (void *)((uintptr_t)task->kstack + PROC_STACK_SIZE);

//     task->elf_size = file->f_inode->i_size;
//     task->elf_image = (char *)kmalloc(task->elf_size);
//     CHECK(task->elf_image != NULL, "task create: alloc elf buffer failed", filp_close(file); task_destroy(task); return NULL;);

//     read_len = kernel_read(file, task->elf_image, task->elf_size);
//     filp_close(file);
//     CHECK(read_len == (ssize_t)task->elf_size, "task create: read elf failed", task_destroy(task); return NULL;);

//     task->elf_info = elf_parse_image(task->elf_image, task->elf_size);
//     CHECK(task->elf_info != NULL, "task create: parse elf failed", task_destroy(task); return NULL;);
//     CHECK(task_prepare_user(task) == 0, "task create: prepare user task failed", task_destroy(task); return NULL;);
//     task_attach_to_rq(task);
//     return task;
// }