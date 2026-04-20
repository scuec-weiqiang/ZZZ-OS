#ifndef _OS_COMPLETION_H
#define _OS_COMPLETION_H

#include <os/wait.h>

struct completion {
    int done;
    struct wait_queue_head wait;
};

void init_completion(struct completion *x);
void wait_for_completion(struct completion *x);
void complete(struct completion *x);
void complete_all(struct completion *x); // 可选

#endif