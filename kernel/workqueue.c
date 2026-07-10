#include <os/workqueue.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/sched.h>
#include <os/init.h>

struct workqueue_struct *system_wq;

static int system_work_thread(void *arg) {
    struct work_struct *next_work = NULL;
    
    while(1) {
        int flags = spin_lock_irqsave(&system_wq->lock);

        if (list_empty(&system_wq->work_list)) {
            spin_unlock_irqrestore(&system_wq->lock,flags);
            sleep_on(&system_wq->wait);
        }

        next_work = container_of(system_wq->work_list.next, struct work_struct, entry);
        list_del(&next_work->entry);
        next_work->pending = 0;
        spin_unlock_irqrestore(&system_wq->lock,flags);

        next_work->func(next_work);
    }
}

void init_work(struct work_struct *work, void (*func)(struct work_struct *)) {
    work->func = func;
    INIT_LIST_HEAD(&work->entry);
    work->pending = 0;
}

struct workqueue_struct *create_workqueue(const char *name) {
    if (!name) {
        return ERR_PTR(-EINVAL);
    }

    struct workqueue_struct *wq_struct = (struct workqueue_struct *)
                    kmalloc(sizeof(struct workqueue_struct));
    if (!wq_struct) {
        return ERR_PTR(-ENOMEM);
    }
    wq_struct->name = name;
    INIT_LIST_HEAD(&wq_struct->work_list);
    init_waitqueue_head(&wq_struct->wait,WAIT_IDLE);
    spin_lock_init(&wq_struct->lock);

    int pid =  kernel_thread(system_work_thread, NULL);
    struct task_struct *wt = find_task_by_pid(pid);
    if (IS_ERR(wt)) {
        kfree(wq_struct);
        return ERR_CAST(wt);
    }

    wq_struct->worker = wt;

    return wq_struct;
}

bool queue_work(struct workqueue_struct *wq, struct work_struct *work) {
    if (!wq || !work) return false;
    if (!wq->worker || !work->func) return false;
    
    if (work->pending) {
       return false;
    }

    spin_lock(&wq->lock);
    list_add_tail(&wq->work_list, &work->entry);
    work->pending = 1;
    spin_unlock(&wq->lock);
    wake_up_process(wq->worker);
    return true;
}

int __init workqueue_init(void) {
    system_wq = create_workqueue("system_wq");
    return 0;
}

core_initcall(workqueue_init);