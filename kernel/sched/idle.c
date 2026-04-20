#include <os/sched.h>

static void idle_enqueue_task(struct rq *rq, struct task_struct *p) {
    // if (rq == NULL || p == NULL) {
    //     return;
    // }

    // if (p->on_rq) {
    //     return;
    // }

    // list_add_tail(&rq->queue, &p->se.sched_node);
    // p->on_rq = 1;
    // rq->nr_running++;
}

static void idle_dequeue_task(struct rq *rq, struct task_struct *p) {
    // if (rq == NULL || p == NULL) {
    //     return;
    // }

    // if (!p->on_rq) {
    //     return;
    // }

    // list_del(&p->se.sched_node);
    // p->on_rq = 0;
    // if (rq->nr_running > 0) {
    //     rq->nr_running--;
    // }
}

static struct task_struct *idle_pick_next_task(struct rq *rq) {
    return rq->idle;
}

// static void idle_task_yield(struct rq *rq, struct task_struct *curr) {
//     if (rq == NULL || curr == NULL) {
//         return;
//     }
//     idle_enqueue_task(rq, curr);
// }

struct sched_class idle_sched_class = {
    .name = "idle",
    .next = NULL,
    .enqueue_task = idle_enqueue_task,
    .dequeue_task = idle_dequeue_task,
    .pick_next_task = idle_pick_next_task,
    // .task_yield = idle_task_yield,
};
