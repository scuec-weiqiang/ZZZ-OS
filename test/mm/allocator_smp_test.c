/* OpenAI Codex 生成：多核绑核分配、交接释放和缓存参数对比。 */
#include <mm/buddy.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <os/check.h>
#include <os/errno.h>
#include <os/init.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/timekeeping.h>

#define MAX_WORKERS 4
static u32 nr_workers;
#define SLOTS 64
#define ROUNDS 128
#define REPEATS 3

static struct {
    u32 command, ready[MAX_WORKERS], done[MAX_WORKERS];
    u32 allocated[MAX_WORKERS], released[MAX_WORKERS];
    u32 kind, slots, remote;
    void *objects[MAX_WORKERS][SLOTS];
    struct kmem_cache *cache;
    u64 elapsed[MAX_WORKERS];
} test;

static u32 load(u32 *value) { return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
static void publish(u32 *value, u32 data) { __atomic_store_n(value, data, __ATOMIC_RELEASE); }

static void wait_value(u32 *value, u32 target)
{
    u64 start = monotonic_ns();
    while (load(value) < target) {
        if (monotonic_ns() - start > 10ULL * NSEC_PER_SEC)
            panic("allocator-smp: timeout cpu=%d target=%u actual=%u\n",
                  get_cpuid(), target, load(value));
        yield();
    }
}

static u32 object_size(void)
{
    return test.kind == 0 ? PAGE_SIZE : test.cache->object_size;
}

static void pattern(void *ptr, u32 cpu, u32 round, u32 slot, bool check)
{
    u8 *data = ptr;
    u32 size = object_size();
    /* 检查头尾各 32 B，避免页面填充成本完全掩盖分配器成本。 */
    for (u32 i = 0; i < 64; i++) {
        u32 offset = i < 32 ? i : size - 64 + i;
        u8 expected = (cpu * 97 + round * 13 + slot * 7 + i);
        if (check) {
            if (data[offset] != expected)
                panic("allocator-smp: corruption cpu=%u round=%u slot=%u offset=%u\n",
                      cpu, round, slot, offset);
        } else {
            data[offset] = expected;
        }
    }
}

static int allocator_worker(void *arg)
{
    u32 cpu = (uintptr_t)arg;
    u32 command = 0;
    ASSERT(get_cpuid() == cpu, "allocator test thread is not bound");
    publish(&test.ready[cpu], 1);
    for (;;) {
        wait_value(&test.command, command + 1);
        command = load(&test.command);
        if (command == UINT32_MAX)
            return 0;
        u64 start = monotonic_ns();
        for (u32 round = 0; round < ROUNDS; round++) {
            ASSERT(get_cpuid() == cpu, "allocator test migrated");
            for (u32 i = 0; i < test.slots; i++) {
                void *obj = test.kind == 0 ? alloc_pages_kva(1) : kmem_cache_alloc(test.cache);
                ASSERT(obj != NULL, "allocator test allocation failed");
                test.objects[cpu][i] = obj;
                pattern(obj, cpu, round, i, false);
            }
            publish(&test.allocated[cpu], round + 1);
            for (u32 peer = 0; peer < nr_workers; peer++)
                wait_value(&test.allocated[peer], round + 1);
            for (u32 i = 0; i < test.slots; i++) {
                for (u32 j = i + 1; j < test.slots; j++)
                    ASSERT(test.objects[cpu][i] != test.objects[cpu][j], "duplicate local allocation");
                for (u32 peer = cpu + 1; peer < nr_workers; peer++)
                    for (u32 j = 0; j < test.slots; j++)
                        ASSERT(test.objects[cpu][i] != test.objects[peer][j], "duplicate cross-CPU allocation");
            }
            u32 owner = test.remote ? (cpu + 1) % nr_workers : cpu;
            for (u32 i = 0; i < test.slots; i++) {
                void *obj = test.objects[owner][i];
                pattern(obj, owner, round, i, true);
                if (test.kind == 0)
                    free_pages_kva(obj);
                else
                    kmem_cache_free(obj);
            }
            publish(&test.released[cpu], round + 1);
            for (u32 peer = 0; peer < nr_workers; peer++)
                wait_value(&test.released[peer], round + 1);
        }
        test.elapsed[cpu] = monotonic_ns() - start;
        publish(&test.done[cpu], command);
    }
}

static u64 free_count(void)
{
    struct buddy_fragmentation_stats stats;
    u64 count = 0;
    buddy_get_fragmentation_stats(&stats);
    for (u32 order = 0; order < MAX_ORDER; order++)
        count += stats.free_blocks[order] << order;
    return count + stats.pcp_free_pages;
}

static int allocator_smp_test_init(void)
{
    const u32 limits[] = {4, 8, 16};
    const u32 highs[] = {16, 32, 64};
    const u32 batches[] = {4, 8, 16};
    struct kmem_cache *caches[2];
    struct pcp_stats original;
    pid_t workers[MAX_WORKERS];
    u32 command = 0;

    while (nr_workers < MAX_WORKERS && cpu_online(nr_workers))
        nr_workers++;
    if (nr_workers < 2) {
        printk("allocator-smp: SKIP requires at least two online CPUs\n");
        return 0;
    }
    caches[0] = kmem_cache_create("smp-test-64", 64, 8);
    caches[1] = kmem_cache_create("smp-test-1024", 1024, 8);
    ASSERT(caches[0] && caches[1], "test cache creation failed");
    pcp_get_stats(0, &original);
    for (u32 cpu = 0; cpu < nr_workers; cpu++) {
        workers[cpu] = kernel_thread_on_cpu(allocator_worker, (void *)(uintptr_t)cpu, cpu);
        ASSERT(workers[cpu] > 0, "test worker creation failed");
    }
    for (u32 cpu = 0; cpu < nr_workers; cpu++)
        wait_value(&test.ready[cpu], 1);

    printk("allocator-smp: START cpus=%u rounds=%u repeats=%u\n", nr_workers, ROUNDS, REPEATS);
    for (u32 repeat = 0; repeat < REPEATS; repeat++) {
        for (u32 step = 0; step < 3; step++) {
            u32 config = (step + repeat) % 3;
            for (u32 kind = 0; kind < 3; kind++) {
                test.kind = kind;
                test.cache = kind ? caches[kind - 1] : NULL;
                if (test.cache)
                    kmem_cache_set_magazine(test.cache, limits[config], limits[config] / 2);
                for (u32 mode = 0; mode < 3; mode++) {
                    struct pcp_stats before[MAX_WORKERS], after[MAX_WORKERS];
                    u64 hits = 0, misses = 0, global = 0, ns = 0;
                    u64 slab_hits[MAX_WORKERS], slab_misses[MAX_WORKERS], slab_global[MAX_WORKERS];
                    u64 retained_pages = 0;
                    test.slots = mode == 0 ? 8 : SLOTS;
                    test.remote = mode == 2;
                    if (test.cache)
                        kmem_cache_drain(test.cache);
                    /* 测 Slab 时固定 PCP 参数，避免两层同时变化影响结论。 */
                    pcp_configure(kind ? 32 : highs[config],
                                  kind ? 8 : batches[config]);
                    u64 baseline = free_count();
                    for (u32 cpu = 0; cpu < nr_workers; cpu++) {
                        publish(&test.allocated[cpu], 0);
                        publish(&test.released[cpu], 0);
                        pcp_get_stats(cpu, &before[cpu]);
                        if (test.cache) {
                            slab_hits[cpu] = test.cache->cpu[cpu].hits;
                            slab_misses[cpu] = test.cache->cpu[cpu].misses;
                            slab_global[cpu] = test.cache->cpu[cpu].global_accesses;
                        }
                    }
                    publish(&test.command, ++command);
                    for (u32 cpu = 0; cpu < nr_workers; cpu++)
                        wait_value(&test.done[cpu], command);
                    for (u32 cpu = 0; cpu < nr_workers; cpu++) {
                        if (test.elapsed[cpu] > ns) ns = test.elapsed[cpu];
                        pcp_get_stats(cpu, &after[cpu]);
                        if (!test.cache) {
                            ASSERT(after[cpu].hits - before[cpu].hits +
                                   after[cpu].misses - before[cpu].misses == (u64)ROUNDS * test.slots,
                                   "per-CPU allocation counters do not match");
                            ASSERT(after[cpu].refills - before[cpu].refills ==
                                   after[cpu].misses - before[cpu].misses,
                                   "per-CPU refill counter is incorrect");
                            retained_pages += after[cpu].count;
                            hits += after[cpu].hits - before[cpu].hits;
                            misses += after[cpu].misses - before[cpu].misses;
                            global += after[cpu].refills - before[cpu].refills +
                                      after[cpu].drains - before[cpu].drains;
                        } else {
                            hits += test.cache->cpu[cpu].hits - slab_hits[cpu];
                            misses += test.cache->cpu[cpu].misses - slab_misses[cpu];
                            global += test.cache->cpu[cpu].global_accesses - slab_global[cpu];
                        }
                    }
                    u64 pairs = (u64)nr_workers * ROUNDS * test.slots;
                    ASSERT(hits + misses == pairs, "allocation counters do not match");
                    if (test.cache) {
                        retained_pages = test.cache->total_slabs;
                        kmem_cache_drain(test.cache);
                        ASSERT(test.cache->total_slabs == 0, "test slabs leaked");
                    }
                    u64 recovered = free_count();
                    ASSERT(recovered == baseline, "allocator test leaked pages");
                    printk("allocator-smp: rep=%u cfg=%u kind=%u mode=%u pairs=%lu hits=%lu misses=%lu global=%lu us=%lu pairs_s=%lu retained_KiB=%lu free_before_KiB=%lu free_after_KiB=%lu recovery=PASS\n",
                           repeat, config, kind, mode, (unsigned long)pairs,
                           (unsigned long)hits, (unsigned long)misses, (unsigned long)global,
                           (unsigned long)(ns / 1000), (unsigned long)(pairs * NSEC_PER_SEC / (ns ? ns : 1)),
                           (unsigned long)(retained_pages * (PAGE_SIZE / 1024)),
                           (unsigned long)(baseline * (PAGE_SIZE / 1024)),
                           (unsigned long)(recovered * (PAGE_SIZE / 1024)));
                }
            }
        }
    }
    publish(&test.command, UINT32_MAX);
    for (u32 cpu = 0; cpu < nr_workers; cpu++) {
        int status;
        do_waitpid(workers[cpu], &status, 0);
    }
    pcp_configure(original.high, original.batch);
    for (u32 i = 0; i < 2; i++)
        free_pages_kva(caches[i]);
    printk("allocator-smp: PASS phases=%u\n", REPEATS * 27);
    return 0;
}

late_initcall(allocator_smp_test_init);
