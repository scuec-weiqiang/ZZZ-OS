#ifndef _OS_WAIT_H
#define _OS_WAIT_H

#include <os/types.h>
#include <os/list.h>

struct wait_queue {
    void *private;
    struct list_head list;
};

struct wait_queue_head {
    struct list_head head;
};

#define WAIT_QUEUE_INIT(name) { .head = LIST_HEAD_INIT((name).head) }

static inline void init_waitqueue_head(struct wait_queue_head *wq_head) {
    INIT_LIST_HEAD(&wq_head->head);
}

static inline int wait_queue_empty(struct wait_queue_head *wq_head) {
    return list_empty(&wq_head->head);
}

static inline void wait_queue_add(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    list_add_tail(&wq_head->head, &wait->list);
}

static inline void wait_queue_remove(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    list_del(&wait->list);
}

#endif