#include <fs/blkdev.h>
#include <fs/types.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/mutex.h>

#include "ext2_types.h"

#define EXT2_INDIRECT_CACHE_SIZE 64

struct ext2_indirect_cache_entry {
    u32 block_no;
    void *data;
    u64 last_used;
    bool valid;
};

struct ext2_indirect_cache {
    struct mutex lock;
    u32 block_size;
    u64 clock;
    u64 hits;
    u64 misses;
    u64 evictions;
    struct ext2_indirect_cache_entry entries[EXT2_INDIRECT_CACHE_SIZE];
};

int ext2_indirect_cache_init(struct ext2_sb_info *sbi, u32 block_size)
{
    struct ext2_indirect_cache *cache;

    if (!sbi || block_size == 0 || block_size > PAGE_SIZE)
        return -EINVAL;

    cache = kzalloc(sizeof(*cache));
    if (!cache)
        return -ENOMEM;

    mutex_init(&cache->lock);
    cache->block_size = block_size;
    sbi->indirect_cache = cache;
    return 0;
}

void ext2_indirect_cache_destroy(struct ext2_sb_info *sbi)
{
    struct ext2_indirect_cache *cache;

    if (!sbi || !sbi->indirect_cache)
        return;

    cache = sbi->indirect_cache;
    for (u32 i = 0; i < EXT2_INDIRECT_CACHE_SIZE; i++)
        page_free(cache->entries[i].data);

    sbi->indirect_cache = NULL;
    kfree(cache);
}

static struct ext2_indirect_cache_entry *
ext2_indirect_cache_find(struct ext2_indirect_cache *cache, u32 block_no)
{
    for (u32 i = 0; i < EXT2_INDIRECT_CACHE_SIZE; i++) {
        struct ext2_indirect_cache_entry *entry = &cache->entries[i];

        if (entry->valid && entry->block_no == block_no)
            return entry;
    }

    return NULL;
}

static struct ext2_indirect_cache_entry *
ext2_indirect_cache_select_victim(struct ext2_indirect_cache *cache)
{
    struct ext2_indirect_cache_entry *victim = &cache->entries[0];

    for (u32 i = 0; i < EXT2_INDIRECT_CACHE_SIZE; i++) {
        struct ext2_indirect_cache_entry *entry = &cache->entries[i];

        if (!entry->valid)
            return entry;
        if (entry->last_used < victim->last_used)
            victim = entry;
    }

    return victim;
}

int ext2_indirect_cache_read(struct super_block *sb, u32 block_no,
                             u32 index, u32 *value)
{
    struct ext2_indirect_cache *cache;
    struct ext2_indirect_cache_entry *entry;
    int ret = 0;

    if (!sb || !value || !EXT2_SB(sb) || !EXT2_SB(sb)->indirect_cache)
        return -EINVAL;

    if (block_no == 0) {
        *value = 0;
        return 0;
    }

    cache = EXT2_SB(sb)->indirect_cache;
    if (index >= cache->block_size / sizeof(u32))
        return -EINVAL;

    mutex_lock(&cache->lock);

    entry = ext2_indirect_cache_find(cache, block_no);
    if (entry) {
        cache->hits++;
    } else {
        entry = ext2_indirect_cache_select_victim(cache);
        if (entry->valid)
            cache->evictions++;

        if (!entry->data) {
            entry->data = page_alloc(1);
            if (!entry->data) {
                ret = -ENOMEM;
                goto out_unlock;
            }
        }

        entry->valid = false;
        ret = blkdev_read(sb->s_bdev, entry->data, cache->block_size,
                          (u64)block_no * cache->block_size);
        if (ret < 0)
            goto out_unlock;

        entry->block_no = block_no;
        entry->valid = true;
        cache->misses++;
    }

    entry->last_used = ++cache->clock;
    *value = ((u32 *)entry->data)[index];

out_unlock:
    mutex_unlock(&cache->lock);
    return ret;
}

int ext2_indirect_cache_write(struct super_block *sb, u32 block_no,
                              const void *data)
{
    struct ext2_indirect_cache *cache;
    struct ext2_indirect_cache_entry *entry;
    int ret;

    if (!sb || !data || block_no == 0 || !EXT2_SB(sb) ||
        !EXT2_SB(sb)->indirect_cache)
        return -EINVAL;

    cache = EXT2_SB(sb)->indirect_cache;
    mutex_lock(&cache->lock);

    entry = ext2_indirect_cache_find(cache, block_no);
    if (entry)
        entry->valid = false;

    ret = blkdev_write(sb->s_bdev, data, cache->block_size,
                       (u64)block_no * cache->block_size);

    mutex_unlock(&cache->lock);
    return ret;
}

void ext2_indirect_cache_invalidate(struct super_block *sb, u32 block_no)
{
    struct ext2_indirect_cache *cache;
    struct ext2_indirect_cache_entry *entry;

    if (!sb || block_no == 0 || !EXT2_SB(sb) ||
        !EXT2_SB(sb)->indirect_cache)
        return;

    cache = EXT2_SB(sb)->indirect_cache;
    mutex_lock(&cache->lock);
    entry = ext2_indirect_cache_find(cache, block_no);
    if (entry)
        entry->valid = false;
    mutex_unlock(&cache->lock);
}
