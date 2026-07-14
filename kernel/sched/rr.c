#include <os/list.h>
#include <os/sched.h>
#include <os/printk.h>
#include <os/bitops.h>
#include <os/timekeeping.h>

static void rr_enqueue_task(struct rq *rq, struct task_struct *p) {
    if (rq == NULL || p == NULL) {
        return;
    }

    if (p->on_rq) {
        return;
    }

    int prio = p->prio;
    bool was_empty = !(rq->prio_bitmap & BIT(prio));

    list_add_tail(&rq->runnable[prio], &p->se.sched_node);
    rq->prio_bitmap |= BIT(prio);
    p->on_rq = 1;
    rq->nr_running++;

    // 队列从空变非空，初始化 clock，避免刚入队就被 aging
    if (was_empty) {
        rq->prio_last_served[prio] = monotonic_ns();
    }
}

static void rr_dequeue_task(struct rq *rq, struct task_struct *p) {
    if (rq == NULL || p == NULL) {
        return;
    }

    if (!p->on_rq) {
        return;
    }

    int prio = p->prio;

    list_del(&p->se.sched_node);
    INIT_LIST_HEAD(&p->se.sched_node);
    if (list_empty(&rq->runnable[prio])) {
        rq->prio_bitmap &= ~BIT(prio);
    }
    p->on_rq = 0;
    if (rq->nr_running > 0) {
        rq->nr_running--;
    }

    // 离开 runqueue 时恢复静态优先级，清零连续 tick 计数
    p->prio = p->base_prio;
    p->se.consec_ticks = 0;
}

static void rr_remove_task(struct rq *rq, struct task_struct *p) {
    int prio;

    if (rq == NULL || p == NULL || !p->on_rq) {
        return;
    }

    prio = p->prio;
    list_del(&p->se.sched_node);
    INIT_LIST_HEAD(&p->se.sched_node);
    if (list_empty(&rq->runnable[prio])) {
        rq->prio_bitmap &= ~BIT(prio);
    }
    p->on_rq = 0;
    if (rq->nr_running > 0) {
        rq->nr_running--;
    }
}

/* 32-bit count trailing zeros — 不依赖 libgcc，bare-metal 安全 */
static inline int ctz32(u32 x)
{
    int r = 0;
    if (!(x & 0x0000FFFF)) { x >>= 16; r += 16; }
    if (!(x & 0x000000FF)) { x >>= 8;  r += 8;  }
    if (!(x & 0x0000000F)) { x >>= 4;  r += 4;  }
    if (!(x & 0x00000003)) { x >>= 2;  r += 2;  }
    if (!(x & 0x00000001)) { r += 1; }
    return r;
}

static struct task_struct *rr_pick_next_task(struct rq *rq) {
    struct list_head *node;

    if (rq == NULL) {
        return NULL;
    }

    if (rq->curr != rq->idle && rq->curr->status == TASK_RUNNING) {
        struct task_struct *curr = rq->curr;
        int curr_prio = curr->prio;

        // 连续跑满 3 个时间片未 sleep → CPU 密集型，降权
        if (curr->se.consec_ticks >= 3 &&
            curr_prio > PRIO_HIGHEST && curr_prio < PRIO_LOWEST) {
            curr->prio = curr_prio + 1;
            curr->se.consec_ticks = 0;
        }

        rr_enqueue_task(rq, curr);
    }

    if (rq->prio_bitmap == 0) {
        return NULL;
    }

    int highest = ctz32(rq->prio_bitmap);
    struct task_struct *p = NULL;
    node = rq->runnable[highest].next;
    p = list_entry(node, struct task_struct, se.sched_node);
    rr_remove_task(rq, p);

    // 记录该优先级最后一次被服务的时间，用于 aging 判断
    rq->prio_last_served[highest] = monotonic_ns();

    return p;
}

/*
 * Aging: 低优先级队列长时间未被服务时，整队逐级提权，防止饥饿。
 * 每次 tick 调用一次，只提升一级（例如 prio 6 → prio 5），渐进式响应。
 */
static void rr_aging(struct rq *rq) {
    u64 now = monotonic_ns();

    // prio 0 预留给实时任务，prio 1 是普通最高优先级
    // aging 从 prio=2 开始，最多提升到 prio=1，绝不触及 prio=0
    for (int prio = 2; prio < PRIO_NUMS; prio++) {
        if (!(rq->prio_bitmap & BIT(prio)))
            continue;  // 队列为空

        if (now - rq->prio_last_served[prio] < AGING_THRESHOLD_NS)
            continue;  // 还没饿够

        int new_prio = prio - 1;

        // 遍历队列中所有 task，同步更新其有效优先级
        struct task_struct *p;
        list_for_each_entry(p, &rq->runnable[prio], se.sched_node) {
            p->prio = new_prio;
        }

        // 整队搬迁：拼接到目标队列尾部
        list_splice_init(&rq->runnable[prio],
                         rq->runnable[new_prio].prev);

        // 更新 bitmap
        rq->prio_bitmap |= BIT(new_prio);
        rq->prio_bitmap &= ~BIT(prio);

        // 重置目标队列时钟，让提上来的 task 有机会被调度
        rq->prio_last_served[new_prio] = now;
    }
}

/*
 * task_tick: 当前任务用完了整个时间片（被 one-shot 时钟中断抢占）。
 * 只做计数，不操作队列（中断上下文，不持有 rq->lock）。
 * 实际降权在 rr_pick_next_task 中、rq->lock 保护下执行。
 */
static void rr_task_tick(struct rq *rq, struct task_struct *curr) {
    if (curr == NULL || curr == rq->idle)
        return;
    curr->se.consec_ticks++;
}

extern struct sched_class idle_sched_class;

struct sched_class rr_sched_class = {
    .name = "rr",
    .next = &idle_sched_class,
    .enqueue_task = rr_enqueue_task,
    .dequeue_task = rr_dequeue_task,
    .pick_next_task = rr_pick_next_task,
    .aging = rr_aging,
    .task_tick = rr_task_tick,
};
