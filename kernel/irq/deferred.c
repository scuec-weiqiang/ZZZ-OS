#include <os/irq.h>

static struct irq_deferred_work *deferred_works[IRQ_DEFERRED_WORK_MAX];

int irq_deferred_work_register(struct irq_deferred_work *work,
                               irq_deferred_fn_t fn, void *arg)
{
    int i;

    if (!work || !fn) {
        return -1;
    }

    for (i = 0; i < IRQ_DEFERRED_WORK_MAX; i++) {
        if (deferred_works[i] == work) {
            work->fn = fn;
            work->arg = arg;
            work->pending = 0;
            return 0;
        }
    }

    for (i = 0; i < IRQ_DEFERRED_WORK_MAX; i++) {
        if (deferred_works[i] == NULL) {
            deferred_works[i] = work;
            work->fn = fn;
            work->arg = arg;
            work->pending = 0;
            return 0;
        }
    }

    return -1;
}

void irq_deferred_work_queue(struct irq_deferred_work *work)
{
    if (!work || !work->fn) {
        return;
    }

    work->pending = 1;
}

void irq_run_deferred_works(void)
{
    int i;

    for (i = 0; i < IRQ_DEFERRED_WORK_MAX; i++) {
        struct irq_deferred_work *work = deferred_works[i];

        if (!work || !work->pending || !work->fn) {
            continue;
        }

        work->pending = 0;
        work->fn(work->arg);
    }
}
