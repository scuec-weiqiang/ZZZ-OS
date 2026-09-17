/*
 * ZZZ-OS allocator correctness test.
 * The summary contains actual operation counts, verified bytes and runtime.
 */

#include <os/errno.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/timekeeping.h>
#include <os/types.h>

#define ALLOC_TEST_OBJECTS 24U
#define ALLOC_TEST_ROUNDS  3U

static const size_t alloc_test_sizes[] = {
    1, 7, 8, 9, 15, 16, 17, 31, 32, 33,
    47, 48, 49, 63, 64, 65, 95, 96, 97,
    127, 128, 129, 191, 192, 193, 255, 256, 257,
    383, 384, 385, 511, 512, 513, 767, 768, 769,
    1023, 1024, 1025, 1535, 1536, 1537,
    2047, 2048, 2049, 4095, 4096, 4097, 8192,
};

static u8 allocator_pattern(size_t size, u32 slot, u32 round, size_t offset)
{
    return (u8)(size * 17U + slot * 131U + round * 29U + offset);
}

static void allocator_fill(void *ptr, size_t size, u32 slot, u32 round)
{
    u8 *data = ptr;

    for (size_t i = 0; i < size; i++)
        data[i] = allocator_pattern(size, slot, round, i);
}

static int allocator_verify(void *ptr, size_t size, u32 slot, u32 round,
                            u64 *verified_bytes)
{
    u8 *data = ptr;

    for (size_t i = 0; i < size; i++) {
        u8 expected = allocator_pattern(size, slot, round, i);

        if (data[i] != expected) {
            printk("allocator-test: FAIL size=%lu slot=%u round=%u "
                   "offset=%lu expected=%u actual=%u\n",
                   (unsigned long)size, slot, round, (unsigned long)i,
                   (unsigned int)expected, (unsigned int)data[i]);
            return -EIO;
        }
    }

    *verified_bytes += size;
    return 0;
}

static int allocator_test_kmalloc(u64 *allocations, u64 *verified_bytes,
                                  u64 *cases)
{
    void *objects[ALLOC_TEST_OBJECTS];

    for (u32 round = 0; round < ALLOC_TEST_ROUNDS; round++) {
        for (u32 size_index = 0;
             size_index < sizeof(alloc_test_sizes) / sizeof(alloc_test_sizes[0]);
             size_index++) {
            size_t size = alloc_test_sizes[size_index];
            int ret = 0;

            for (u32 i = 0; i < ALLOC_TEST_OBJECTS; i++)
                objects[i] = NULL;

            for (u32 i = 0; i < ALLOC_TEST_OBJECTS; i++) {
                objects[i] = kmalloc(size);
                if (!objects[i]) {
                    printk("allocator-test: FAIL allocation size=%lu slot=%u\n",
                           (unsigned long)size, i);
                    ret = -ENOMEM;
                    goto free_objects;
                }
                if ((uintptr_t)objects[i] & 7U) {
                    printk("allocator-test: FAIL alignment size=%lu slot=%u ptr=%lx\n",
                           (unsigned long)size, i,
                           (unsigned long)objects[i]);
                    ret = -EIO;
                    goto free_objects;
                }

                for (u32 previous = 0; previous < i; previous++) {
                    if (objects[previous] == objects[i]) {
                        printk("allocator-test: FAIL duplicate size=%lu slots=%u,%u\n",
                               (unsigned long)size, previous, i);
                        ret = -EIO;
                        goto free_objects;
                    }
                }

                allocator_fill(objects[i], size, i, round);
                (*allocations)++;
            }

            for (u32 i = 0; i < ALLOC_TEST_OBJECTS; i++) {
                ret = allocator_verify(objects[i], size, i, round,
                                       verified_bytes);
                if (ret < 0)
                    goto free_objects;
            }

            /* Create holes in each slab/magazine, then allocate into them. */
            for (u32 i = 1; i < ALLOC_TEST_OBJECTS; i += 2) {
                kfree(objects[i]);
                objects[i] = NULL;
            }

            for (u32 i = 0; i < ALLOC_TEST_OBJECTS; i += 2) {
                ret = allocator_verify(objects[i], size, i, round,
                                       verified_bytes);
                if (ret < 0)
                    goto free_objects;
            }

            for (u32 i = 1; i < ALLOC_TEST_OBJECTS; i += 2) {
                objects[i] = kmalloc(size);
                if (!objects[i]) {
                    ret = -ENOMEM;
                    goto free_objects;
                }
                allocator_fill(objects[i], size, i, round);
                (*allocations)++;
            }

            for (u32 i = 0; i < ALLOC_TEST_OBJECTS; i++) {
                ret = allocator_verify(objects[i], size, i, round,
                                       verified_bytes);
                if (ret < 0)
                    goto free_objects;
            }

            (*cases)++;

free_objects:
            for (u32 i = ALLOC_TEST_OBJECTS; i > 0; i--) {
                if (objects[i - 1]) {
                    kfree(objects[i - 1]);
                    objects[i - 1] = NULL;
                }
            }
            if (ret < 0)
                return ret;
        }
    }

    return 0;
}

static int allocator_test_pages(u64 *allocations, u64 *verified_bytes,
                                u64 *cases)
{
    static const size_t page_counts[] = { 1, 2, 3, 4, 7, 8, 9 };

    for (u32 i = 0; i < sizeof(page_counts) / sizeof(page_counts[0]); i++) {
        size_t bytes = page_counts[i] * PAGE_SIZE;
        u8 *memory = page_alloc(page_counts[i]);

        if (!memory)
            return -ENOMEM;
        if ((uintptr_t)memory & (PAGE_SIZE - 1)) {
            printk("allocator-test: FAIL page_alignment pages=%lu ptr=%lx\n",
                   (unsigned long)page_counts[i], (unsigned long)memory);
            page_free(memory);
            return -EIO;
        }

        for (size_t offset = 0; offset < bytes; offset++)
            memory[offset] = (u8)(i * 37U + offset);
        for (size_t offset = 0; offset < bytes; offset++) {
            u8 expected = (u8)(i * 37U + offset);

            if (memory[offset] != expected) {
                printk("allocator-test: FAIL pages=%lu offset=%lu "
                       "expected=%u actual=%u\n",
                       (unsigned long)page_counts[i], (unsigned long)offset,
                       (unsigned int)expected, (unsigned int)memory[offset]);
                page_free(memory);
                return -EIO;
            }
        }

        (*allocations)++;
        *verified_bytes += bytes;
        (*cases)++;
        page_free(memory);
    }

    return 0;
}

static int allocator_test_init(void)
{
    u64 allocations = 0;
    u64 verified_bytes = 0;
    u64 cases = 0;
    u64 start_ns = monotonic_ns();
    int ret;

    printk("allocator-test: START objects=%u rounds=%u\n",
           ALLOC_TEST_OBJECTS, ALLOC_TEST_ROUNDS);

    ret = allocator_test_kmalloc(&allocations, &verified_bytes, &cases);
    if (ret == 0)
        ret = allocator_test_pages(&allocations, &verified_bytes, &cases);

    if (ret < 0) {
        printk("allocator-test: FAIL ret=%d cases=%lu allocations=%lu "
               "verified_bytes=%lu time_us=%lu\n",
               ret, (unsigned long)cases, (unsigned long)allocations,
               (unsigned long)verified_bytes,
               (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
        return ret;
    }

    printk("allocator-test: PASS cases=%lu allocations=%lu "
           "verified_bytes=%lu time_us=%lu\n",
           (unsigned long)cases, (unsigned long)allocations,
           (unsigned long)verified_bytes,
           (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
    return 0;
}

late_initcall(allocator_test_init);
