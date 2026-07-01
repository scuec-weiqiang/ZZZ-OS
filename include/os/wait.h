#ifndef _OS_WAIT_H
#define _OS_WAIT_H

#include <os/types.h>
#include <os/list.h>
#include <os/spinlock.h>

struct wait_queue {
    void *private;
    struct list_head list;
};

struct wait_queue_head {
    spinlock_t lock;
    struct list_head head;
};

#define WAIT_QUEUE_INIT(name) { .lock = SPINLOCK_INIT, .head = LIST_HEAD_INIT((name).head) }

static inline void init_waitqueue_head(struct wait_queue_head *wq_head) {
    spin_lock_init(&wq_head->lock);
    INIT_LIST_HEAD(&wq_head->head);
}

static inline int wait_queue_empty(struct wait_queue_head *wq_head) {
    // spin_lock(&wq_head->lock);
    int empty = list_empty(&wq_head->head);
    // spin_unlock(&wq_head->lock);
    return empty;
}

static inline void wait_queue_add(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    // spin_lock(&wq_head->lock);
    list_add_tail(&wq_head->head, &wait->list);
    // spin_unlock(&wq_head->lock);
}

static inline void wait_queue_remove(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    // spin_lock(&wq_head->lock);
    list_del(&wait->list);
    // spin_unlock(&wq_head->lock);
}

#endif