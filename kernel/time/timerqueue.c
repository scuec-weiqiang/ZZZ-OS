#include <os/timerqueue.h>
#include <os/timekeeping.h>
#include <os/cpu.h>
#include <os/of_cpu.h>
#include <os/printk.h>
#include <os/kmalloc.h>
#include <os/spinlock.h>

struct timer_base {
    spinlock_t lock;
    struct timer *heap[MAX_TIMERS];
    int heap_size;
    uint64_t next_deadline;
};

static struct timer_base *timer_bases;

struct timer_base *get_timer_base(int cpu) {
    if (cpu < 0 || cpu >= of_get_cpu_num()) {
        return NULL;
    }
    return &timer_bases[cpu];
}

struct timer_base *local_timer_base(void) {
    int cpuid = get_cpuid();
    return get_timer_base(cpuid);
}


// 选择合适的 CPU 来运行定时器，均衡负载
static int choose_proper_cpuid() {
    int cpuid = 0;
    unsigned int mini = MAX_TIMERS;
    for (int i = 0; i < of_get_cpu_num(); i++) {
        if (get_timer_base(i)->heap_size < mini) {
            mini = get_timer_base(i)->heap_size;
            cpuid = i;
        }
    }
    return cpuid;
}

static int get_parent_idx(int idx) {
    return (idx - 1) / 2;
}

static int get_left_child_idx(int idx) {
    return 2 * idx + 1;
}

static int get_right_child_idx(int idx) {
    return 2 * idx + 2;
}

static struct timer *heap_top(void) {
    if (local_timer_base()->heap_size == 0) {
        return NULL;
    }
    return local_timer_base()->heap[0];
}

static void swap_heap_nodes(struct timer_base *tb ,int idx1, int idx2) {
    struct timer *temp = tb->heap[idx1];
    tb->heap[idx1] = tb->heap[idx2];
    tb->heap[idx2] = temp;

    tb->heap[idx1]->heap_idx = idx1;
    tb->heap[idx2]->heap_idx = idx2;
}

static void heapify_down(struct timer_base *tb, int idx) {
    int smallest_idx = idx;
    int left_idx = get_left_child_idx(idx);
    int right_idx = get_right_child_idx(idx);

    if (left_idx < tb->heap_size && tb->heap[left_idx]->expires_ns < tb->heap[smallest_idx]->expires_ns) {
        smallest_idx = left_idx;
    }
    if (right_idx < tb->heap_size && tb->heap[right_idx]->expires_ns < tb->heap[smallest_idx]->expires_ns) {
        smallest_idx = right_idx;
    }
    if (smallest_idx != idx) {
        swap_heap_nodes(tb, idx, smallest_idx);
        heapify_down(tb, smallest_idx);
    }
}

static void heapify_up(struct timer_base *tb, int idx) {
    int parent_idx = get_parent_idx(idx);
    if (parent_idx >= 0 && tb->heap[idx]->expires_ns < tb->heap[parent_idx]->expires_ns) {
        swap_heap_nodes(tb, idx, parent_idx);
        heapify_up(tb, parent_idx);
    }
}

static int heap_push(struct timer *t) {
    struct timer_base *tb;
    int cpuid = -1;
    if (t->pinned != -1 && t->pinned != get_cpuid()) {
        cpuid = t->pinned;
    } else {
        cpuid = choose_proper_cpuid();
    }
    t->cpu = cpuid;
    tb = get_timer_base(cpuid);
    tb->heap[tb->heap_size] = t;
    t->heap_idx = tb->heap_size;
    tb->heap_size++;
    
    heapify_up(tb, t->heap_idx);

    return 0;
}

static struct timer *heap_pop(void) {
    struct timer_base *tb = local_timer_base();
    if (tb->heap_size == 0) {
        return NULL;
    }

    struct timer *top = tb->heap[0];
    tb->heap_size--;
    if (tb->heap_size > 0) {
        tb->heap[0] = tb->heap[tb->heap_size];
        tb->heap[0]->heap_idx = 0;

        heapify_down(tb, 0);
    }

    return top;
}

int timerqueue_init(void) {
    int cpu_num = of_get_cpu_num();
    timer_bases = (struct timer_base *)kmalloc(sizeof(struct timer_base) * cpu_num);
    for (int i = 0; i < cpu_num; i++) {
        spin_lock_init(&timer_bases[i].lock);
        timer_bases[i].heap_size = 0;
        timer_bases[i].next_deadline = 0;
    }

    return 0;
}

int timer_start(struct timer *t) {
    if (t->active) {
        return -1;
    }
    t->active = true;
    heap_push(t);
    if (t == heap_top()) {
        program_next_event(t->expires_ns, monotonic_ns());
    }

    return 0;
}

int timer_cancel(struct timer *t) {
    if (!t->active) {
        return -1;
    }
    struct timer_base *tb = get_timer_base(t->cpu);
    t->active = 0;
    int idx = t->heap_idx;
    if (idx >= tb->heap_size || tb->heap[idx] != t) {
        dprintk("timerqueue: invalid timer cancel\n");
        return -1;
    }

    tb->heap_size--;
    if (idx < tb->heap_size) {
        tb->heap[idx] = tb->heap[tb->heap_size];
        tb->heap[idx]->heap_idx = idx;

        heapify_up(tb, idx);
    }

    return 0;
}



uint64_t timerqueue_next_deadline(void) {
    struct timer *t = heap_top();
    if (t) {
        return t->expires_ns;
    }
    return UINT64_MAX; // 没有定时器时，设置一个较短的默认超时时间，避免长时间不响应新定时器的设置
}

void timerqueue_run_expired(uint64_t now) {
    while (true) {
        struct timer *t = heap_top();
        if (!t || t->expires_ns > now) {
            break;
        }
        heap_pop();
        t->callback(t, t->arg);
        if (t->period_ns > 0) {
            t->expires_ns = now + t->period_ns;
            heap_push(t);
        } else {
            t->active = false;
        }
    }
}

int timerqueue_empty(void) {
    return heap_top() == NULL;
}

static void timer1_callback(struct timer *t, void *arg) {
    printk("Timer 1 expired\n");
}

static void timer2_callback(struct timer *t, void *arg) {
    printk("Timer 2 expired\n");
}

struct timer t1 = {
    .callback = timer1_callback,
    .arg = NULL,
    .active = 0,
};

struct timer t2 = {
    .callback = timer2_callback,
    .arg = NULL,
    .active = 0,
};

void timerqueue_test(void) {
    t1.expires_ns = monotonic_ns() + 1000000000; // 1秒后到期
    t1.period_ns = 0; // one-shot

    t2.expires_ns = monotonic_ns() + 2000000000; // 2秒后到期
    t2.period_ns = 0; // 2s周期
    timer_start(&t1);
    timer_start(&t2);
}
