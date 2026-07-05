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
    const char *wait_reason;
    struct list_head head;
};

#define WAIT_QUEUE_INIT(name) { \
    .lock = SPINLOCK_INIT, \
    .head = LIST_HEAD_INIT((name).head), \
}

static inline void init_waitqueue_head(struct wait_queue_head *wq_head) {
    spin_lock_init(&wq_head->lock);
    INIT_LIST_HEAD(&wq_head->head);
}

static inline int wait_queue_empty(struct wait_queue_head *wq_head) {
    int empty = list_empty(&wq_head->head);
    return empty;
}

static inline void wait_queue_add(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    list_add_tail(&wq_head->head, &wait->list);
}

static inline void wait_queue_remove(struct wait_queue_head *wq_head, struct wait_queue *wait) {
    list_del(&wait->list);
}


#define X(name) WAIT_##name,

#define WAIT_REASON_LIST \
    X(NONE) \
    X(CHILD) \
    X(IDLE) \
    X(FUTEX) \
    X(COMPLETION) \
    X(PIPE_READ) \
    X(PIPE_WRITE) \
    X(TTY_READ) \
    X(TTY_WRITE) \
    X(SEM) \
    X(MUTEX) \
    X(CONDVAR) \
    X(EVENT) \
    X(SIGNAL) \
    X(OTHER)

enum {
    WAIT_REASON_LIST
};
#undef X

#define X(name) #name,
static const char *wait_reason_names[] = {
    WAIT_REASON_LIST
};
#undef X


static inline const char *get_wait_reason_name(int reason) {
    if (reason < 0 || reason >= sizeof(wait_reason_names) / sizeof(wait_reason_names[0])) {
        return "UNKNOWN";
    }
    return wait_reason_names[reason];
}

// #define set_wait_reason(task, reason) do { \
//     if (task) { \
//         task->wait_reason = wait_reason[reason]; \
//     } \
// } while(0)



#endif