#include <os/list.h>
#include <os/sched.h>
#include <os/printk.h>

static void rr_enqueue_task(struct rq *rq, struct task_struct *p) {
    if (rq == NULL || p == NULL) {
        return;
    }

    if (p->on_rq) {
        return;
    }

    list_add_tail(&rq->runnable, &p->se.sched_node);
    p->on_rq = 1;
    rq->nr_running++;
}

static void rr_dequeue_task(struct rq *rq, struct task_struct *p) {
    if (rq == NULL || p == NULL) {
        return;
    }

    if (!p->on_rq) {
        return;
    }

    list_del(&p->se.sched_node);
    INIT_LIST_HEAD(&p->se.sched_node);
    p->on_rq = 0;
    if (rq->nr_running > 0) {
        rq->nr_running--;
    }
}

static struct task_struct *rr_pick_next_task(struct rq *rq) {
    struct list_head *node;

    if (rq == NULL || list_empty(&rq->runnable)) {
        return NULL;
    }

    if (this_rq()->curr != rq->idle && this_rq()->curr->status == TASK_RUNNING) {
        // 非空闲任务被抢占，放回队列尾部
        list_mov_tail(&rq->runnable, &this_rq()->curr->se.sched_node);
    }
    
    node = rq->runnable.next;
    struct task_struct *p = list_entry(node, struct task_struct, se.sched_node);
    // printk("rr_pick_next_task: pick task pid %du\n", p->pid);
    return p;
}

// static void rr_task_yield(struct rq *rq, struct task_struct *curr) {
//     if (rq == NULL || curr == NULL) {
//         return;
//     }
//     rr_enqueue_task(rq, curr);
// }
extern struct sched_class idle_sched_class;

struct sched_class rr_sched_class = {
    .name = "rr",
    .next = &idle_sched_class,
    .enqueue_task = rr_enqueue_task,
    .dequeue_task = rr_dequeue_task,
    .pick_next_task = rr_pick_next_task,
    // .task_yield = rr_task_yield,
};
