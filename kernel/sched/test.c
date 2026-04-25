#include <os/check.h>
#include <os/cpu.h>
#include <os/printk.h>
#include <os/sched.h>

struct sched_kthread_test_arg {
    const char *name;
    int rounds;
};

static struct sched_kthread_test_arg sched_test_args[] = {
    { .name = "kthread-A", .rounds = 6 },
    { .name = "kthread-B", .rounds = 6 },
};

static int sched_kthread_test_started;

static int sched_kthread_test_worker(void *arg)
{

    struct sched_kthread_test_arg *cfg = (struct sched_kthread_test_arg *)arg;
    struct task_struct *task = current;

    CHECK(cfg != NULL, "sched test: worker arg is NULL", panic("sched test worker failed\n"););
    CHECK(task != NULL, "sched test: current task is NULL", panic("sched test worker failed\n"););

    for (int i = 0; i < cfg->rounds; i++) {
        printk("[sched-test] cpu=%du pid=%xu task=%s round=%d/%d\n",
               get_cpuid(), (unsigned long)task->pid, cfg->name, i + 1, cfg->rounds);
        yield();
    }

    // while(1) {
    //     printk("[sched-test] cpu=%du pid=%xu task=%s round=%d\n",
    //            get_cpuid(), (unsigned long)task->pid, cfg->name, cfg->rounds);
    //     int a = 0;
    //     for (int i = 0; i < 20000000; i++) {
    //         a += i;
    //     }
        
    // }

    return 0;
}
void sched_kthread_test(void)
{
    struct task_struct *task_a, *task_b;


    CHECK(global_rq != NULL, "sched test: scheduler is not initialized", return;);
    if (sched_kthread_test_started) {
        printk("[sched-test] already started, skip duplicate launch\n");
        return;
    }

    task_a =  kthread_create(sched_kthread_test_worker, &sched_test_args[0]);
    task_b =  kthread_create(sched_kthread_test_worker, &sched_test_args[1]);

    sched_kthread_test_started = 1;
    printk("[sched-test] created kernel threads: A pid=%xu, B pid=%xu\n",
           task_a->pid, task_b->pid);
    
}
