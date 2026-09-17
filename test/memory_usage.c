#include <mm/buddy.h>
#include <os/pfn.h>
#include <os/printk.h>

#include "memory_usage.h"

static u64 pages_to_kib(u64 pages)
{
    return pages * (PAGE_SIZE / 1024U);
}

void test_memory_usage_report(const char *phase)
{
    struct buddy_memory_stats stats;
    u64 free_pages;

    if (buddy_get_memory_stats(&stats) < 0) {
        printk("memory-usage: phase=%s FAIL\n", phase);
        return;
    }

    free_pages = stats.buddy_free_pages + stats.pcp_free_pages;
    printk("memory-usage: phase=%s total_KiB=%lu reserved_KiB=%lu "
           "managed_KiB=%lu used_KiB=%lu free_KiB=%lu\n",
           phase,
           (unsigned long)pages_to_kib(stats.total_pages),
           (unsigned long)pages_to_kib(stats.reserved_pages),
           (unsigned long)pages_to_kib(stats.managed_pages),
           (unsigned long)pages_to_kib(stats.used_pages),
           (unsigned long)pages_to_kib(free_pages));
    printk("memory-usage-detail: phase=%s buddy_free_KiB=%lu "
           "pcp_free_KiB=%lu slab_KiB=%lu pagecache_KiB=%lu\n",
           phase,
           (unsigned long)pages_to_kib(stats.buddy_free_pages),
           (unsigned long)pages_to_kib(stats.pcp_free_pages),
           (unsigned long)pages_to_kib(stats.slab_pages),
           (unsigned long)pages_to_kib(stats.pagecache_pages));
}
