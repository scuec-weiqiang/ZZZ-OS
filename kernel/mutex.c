#include <os/mutex.h>
#include <os/sched.h>

void mutex_init(struct mutex *m) {
    spin_lock_init(&m->lock);
    m->locked = 0;
    init_waitqueue_head(&m->wait, WAIT_MUTEX);
}

int mutex_trylock(struct mutex *m) {
    unsigned long flags;
    int ret = 0;

    flags = spin_lock_irqsave(&m->lock);
    if (!m->locked) {
        m->locked = 1;
        ret = 1;
    }
    spin_unlock_irqrestore(&m->lock, flags);

    return ret;
}

void mutex_lock(struct mutex *m) {
    unsigned long flags;
    struct task_struct *task = current;

    for (;;) {
        flags = spin_lock_irqsave(&m->lock);

        if (!m->locked) {
            m->locked = 1;
            spin_unlock_irqrestore(&m->lock, flags);
            return;
        }

        task->status = TASK_SLEEPING;
        task->wait.private = task;
        wait_queue_add(&m->wait, &task->wait);

        spin_unlock_irqrestore(&m->lock, flags);

        sched();
        // 被唤醒后重新竞争。
    }
}

void mutex_unlock(struct mutex *m) {
    unsigned long flags;

    flags = spin_lock_irqsave(&m->lock);

    if (!m->locked) {
        spin_unlock_irqrestore(&m->lock, flags);
        return;
    }

    m->locked = 0;
    wake_up_one(&m->wait);

    spin_unlock_irqrestore(&m->lock, flags);
}
