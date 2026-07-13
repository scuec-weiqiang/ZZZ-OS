#ifndef __OS_MUTEX_H
#define __OS_MUTEX_H

#include <os/spinlock.h>
#include <os/wait.h>

struct mutex {
    spinlock_t lock;
    int locked;
    struct wait_queue_head wait;
};

void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
int mutex_trylock(struct mutex *m);
void mutex_unlock(struct mutex *m);


#endif /* __OS_MUTEX_H */