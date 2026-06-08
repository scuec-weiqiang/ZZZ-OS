/**
 * 调度器优先级测试：task_tick 降权 + 交互型 vs CPU 密集型对比
 * 独立测试文件，不侵入核心调度代码。
 *
 * 方式一（自动）：开机 late_initcall 自动运行，注释掉不需要的
 * 方式二（手动）：在 shell 里直接调用 sched_deboost_test() / sched_interactive_test()
 */

#include <os/printk.h>
#include <os/sched.h>
#include <os/timekeeping.h>
#include <os/init.h>

/* ---------- 工具函数 ---------- */

/* 忙等约 ms 毫秒，不主动让出 CPU，期间可被时钟中断抢占 */
static void spin_ms(int ms)
{
    u64 end = monotonic_ns() + (u64)ms * 1000000ULL;
    while (monotonic_ns() < end)
        /* spin */;
}

/* ================================================================
 * Test 1: task_tick 降权
 *
 * 纯 CPU 密集型任务，每次用完 10ms 时间片触发 task_tick，
 * prio 逐步递增。预期输出类似:
 *   [deboost] cpu-hog START prio=1
 *   [deboost] cpu-hog round=1 prio=2
 *   [deboost] cpu-hog round=2 prio=3
 *   ...
 * ================================================================ */
static int deboost_worker(void *arg)
{
    struct task_struct *task = current;
    const char *name = (const char *)arg;

    printk("[deboost] %s pid=%d START prio=%d base=%d\n",
           name, task->pid, task->prio, task->base_prio);

    /*
     * 连续跑满 3 个时间片 (30ms) 才降一级，15 轮 = 5 次降权。
     * 预期: prio 1→2→3→4→5→6
     */
    for (int i = 0; i < 15; i++) {
        spin_ms(15);  /* > 10ms 时间片，会触发 sched_event → task_tick */
        printk("[deboost] %s pid=%d round=%d prio=%d\n",
               name, task->pid, i + 1, task->prio);
    }

    printk("[deboost] %s pid=%d DONE prio=%d\n",
           name, task->pid, task->prio);
    return 0;
}

void sched_deboost_test(void)
{
    kthread_create(deboost_worker, "cpu-hog");
    printk("[deboost] test started (expect prio 1→2→3→4→5→6, 3 ticks/step)\n");
}

/* ================================================================
 * Test 2: 交互型 vs CPU 密集型
 *
 * interactive_worker: 频繁 yield → dequeue_task 每次复位 prio=1
 * cpu_bound_worker:   忙等不睡眠 → task_tick 不断降权
 *
 * 预期: interactive 始终 prio=1, cpu-bound 被越降越低。
 * 同时因为 interactive 优先级更高，它的 printk 会穿插在 cpu-bound
 * 的忙等间隙中先输出。
 * ================================================================ */
static int interactive_worker(void *arg)
{
    struct task_struct *task = current;

    for (int i = 0; i < 10; i++) {
        printk("[interact] pid=%d round=%d prio=%d\n",
               task->pid, i, task->prio);
        yield();  /* 主动让出 CPU → dequeue_task → prio = base_prio = 1 */
    }

    printk("[interact] pid=%d DONE prio=%d\n", task->pid, task->prio);
    return 0;
}

static int cpu_bound_worker(void *arg)
{
    struct task_struct *task = current;

    for (int i = 0; i < 8; i++) {
        spin_ms(15);
        printk("[cpu-bound] pid=%d round=%d prio=%d\n",
               task->pid, i + 1, task->prio);
    }

    printk("[cpu-bound] pid=%d DONE prio=%d\n", task->pid, task->prio);
    return 0;
}

void sched_interactive_test(void)
{
    kthread_create(cpu_bound_worker, NULL);
    kthread_create(interactive_worker, NULL);
    printk("[interact] test started "
           "(expect interact-prio=1 always, cpu-bound-prio rising)\n");
}

/* ---------- late_initcall 自动测试入口 ---------- */
static int auto_deboost_test(void)
{
    sched_deboost_test();
    return 0;
}
late_initcall(auto_deboost_test);

/*
 * 如果要测试交互型 vs CPU 密集型，注释掉上面的 late_initcall，
 * 启用下面这个：
 *
 * static int auto_interactive_test(void)
 * {
 *     sched_interactive_test();
 *     return 0;
 * }
 * late_initcall(auto_interactive_test);
 */
