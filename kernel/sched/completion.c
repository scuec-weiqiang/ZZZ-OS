#include <os/completion.h>
#include <os/sched.h>
#include <os/printk.h>

void init_completion(struct completion *x)
{
    x->done = 0;
    init_waitqueue_head(&x->wait);
}

void wait_for_completion(struct completion *x)
{
    while (1) {
        if (x->done > 0) {
            x->done--;
            here;
            return;
        }
        here;
        sleep_on(&x->wait);
    }
}

void complete(struct completion *x)
{
    x->done++;

    wake_up_one(&x->wait);
}


void complete_all(struct completion *x)
{
    x->done = INT_MAX;

    wake_up_all(&x->wait);
}
