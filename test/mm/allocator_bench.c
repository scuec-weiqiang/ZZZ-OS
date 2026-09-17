/* Slab/Buddy microbenchmark. Results include real operation counts. */

#include <os/errno.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/timekeeping.h>
#include <os/types.h>

#define ALLOC_BENCH_SINGLE_OPS   20000U
#define ALLOC_BENCH_BATCH_SIZE   32U
#define ALLOC_BENCH_BATCH_ROUNDS 500U

static int allocator_bench_single(size_t size)
{
    u64 start_ns = monotonic_ns();
    u64 elapsed_ns;

    for (u32 i = 0; i < ALLOC_BENCH_SINGLE_OPS; i++) {
        u8 *ptr = kmalloc(size);

        if (!ptr)
            return -ENOMEM;
        ptr[0] = (u8)i;
        ptr[size - 1] = (u8)(i >> 8);
        kfree(ptr);
    }

    elapsed_ns = monotonic_ns() - start_ns;
    if (elapsed_ns == 0)
        elapsed_ns = 1;
    printk("allocator-bench: mode=single size=%lu alloc_free_pairs=%u "
           "time_us=%lu ns_per_pair=%lu pairs_per_sec=%lu\n",
           (unsigned long)size, ALLOC_BENCH_SINGLE_OPS,
           (unsigned long)(elapsed_ns / 1000ULL),
           (unsigned long)(elapsed_ns / ALLOC_BENCH_SINGLE_OPS),
           (unsigned long)((u64)ALLOC_BENCH_SINGLE_OPS * NSEC_PER_SEC /
                           elapsed_ns));
    return 0;
}

static int allocator_bench_batch(size_t size)
{
    void *objects[ALLOC_BENCH_BATCH_SIZE];
    u64 operations = (u64)ALLOC_BENCH_BATCH_SIZE *
                     ALLOC_BENCH_BATCH_ROUNDS;
    u64 start_ns = monotonic_ns();
    u64 elapsed_ns;

    for (u32 round = 0; round < ALLOC_BENCH_BATCH_ROUNDS; round++) {
        for (u32 i = 0; i < ALLOC_BENCH_BATCH_SIZE; i++) {
            u8 *ptr = kmalloc(size);

            if (!ptr) {
                while (i > 0)
                    kfree(objects[--i]);
                return -ENOMEM;
            }
            ptr[0] = (u8)i;
            ptr[size - 1] = (u8)round;
            objects[i] = ptr;
        }
        for (u32 i = ALLOC_BENCH_BATCH_SIZE; i > 0; i--)
            kfree(objects[i - 1]);
    }

    elapsed_ns = monotonic_ns() - start_ns;
    if (elapsed_ns == 0)
        elapsed_ns = 1;
    printk("allocator-bench: mode=batch size=%lu batch=%u "
           "alloc_free_pairs=%lu time_us=%lu ns_per_pair=%lu "
           "pairs_per_sec=%lu\n",
           (unsigned long)size, ALLOC_BENCH_BATCH_SIZE,
           (unsigned long)operations,
           (unsigned long)(elapsed_ns / 1000ULL),
           (unsigned long)(elapsed_ns / operations),
           (unsigned long)(operations * NSEC_PER_SEC / elapsed_ns));
    return 0;
}

static int allocator_bench_init(void)
{
    static const size_t sizes[] = { 64, 256, 1024, 4096 };
    int ret;

    printk("allocator-bench: START single_ops=%u batch=%u batch_rounds=%u\n",
           ALLOC_BENCH_SINGLE_OPS, ALLOC_BENCH_BATCH_SIZE,
           ALLOC_BENCH_BATCH_ROUNDS);
    for (u32 i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        ret = allocator_bench_single(sizes[i]);
        if (ret < 0)
            return ret;
        ret = allocator_bench_batch(sizes[i]);
        if (ret < 0)
            return ret;
    }
    printk("allocator-bench: PASS\n");
    return 0;
}

late_initcall(allocator_bench_init);
