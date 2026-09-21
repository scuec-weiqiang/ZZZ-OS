#include <fs/pagecache.h>
#include <os/check.h>
#include <os/container_of.h>
#include <os/errno.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/lru.h>
#include <os/string.h>
#include <mm/buddy.h>

#define PAGECACHE_BUCKET_HINT 256
#define PAGECACHE_MAX_PAGES   256

static struct lru_cache *g_pagecache;

static hval_t pagecache_hash(const struct hlist_node *node)
{
    const struct page *page = container_of(node, struct page, cache_lru_node.hnode);
    uintptr_t mapping_ptr = (uintptr_t)page->mapping;
    const u32 golden_ratio = 0x9E3779B9U;
    hval_t hash = (hval_t)mapping_ptr * golden_ratio;

    hash ^= (hval_t)page->index * golden_ratio;
    hash ^= (hash >> 16);
    return hash;
}

static int pagecache_compare(const struct hlist_node *a, const struct hlist_node *b)
{
    const struct page *page_a = container_of(a, struct page, cache_lru_node.hnode);
    const struct page *page_b = container_of(b, struct page, cache_lru_node.hnode);

    if (page_a->mapping == page_b->mapping && page_a->index == page_b->index) {
        return 0;
    }
    return 1;
}

static int pagecache_can_writeback(struct page *page)
{
    return page != NULL &&
           page->mapping != NULL &&
           page->mapping->a_ops != NULL &&
           page->mapping->a_ops->writepage != NULL;
}

static int pagecache_sync_page(struct lru_node *node)
{
    struct page *page = container_of(node, struct page, cache_lru_node);
    int ret = 0;

    ASSERT(page != NULL, "pagecache: invalid page node");

    lock_page(page);
    if (PageDirty(page)) {
        ASSERT(pagecache_can_writeback(page), "pagecache: dirty page has no writeback op");

        SetPageWriteback(page);
        ret = page->mapping->a_ops->writepage(page);
        if (ret == 0) {
            ClearPageDirty(page);
            SetPageUptodate(page);
        }
        ClearPageWriteback(page);
    }
    unlock_page(page);
    return ret;
}

static int pagecache_free_page(struct lru_node *node)
{
    struct page *page = container_of(node, struct page, cache_lru_node);

    ASSERT(page != NULL, "pagecache: invalid free page");

    if (page->mapping != NULL) {
        spin_lock(&page->mapping->lock);
        if (page->mapping->nrpages > 0) {
            page->mapping->nrpages--;
        }
        spin_unlock(&page->mapping->lock);
    }

    page->mapping = NULL;
    page->index = 0;
    page->private = NULL;
    page->refcount = 0;
    page->dirty_blocks = 0;

    page_clear_flag(page, PAGE_LOCKED | PAGE_UPTODATE |
                          PAGE_DIRTY | PAGE_WRITEBACK);
    lru_node_reset(&page->cache_lru_node);
    free_pages(page);
    return 0;
}

static struct lru_ops pagecache_lru_ops = {
    .free = pagecache_free_page,
    .sync = pagecache_sync_page,
};

static struct hash_ops pagecache_hash_ops = {
    .hash_func = pagecache_hash,
    .hash_compare = pagecache_compare,
};

static struct page *pagecache_lookup_nolock(struct address_space *mapping, pgoff_t index)
{
    struct page key;
    struct lru_node *found = NULL;

    memset(&key, 0, sizeof(key));
    key.mapping = mapping;
    key.index = index;
    lru_node_reset(&key.cache_lru_node);

    found = lru_cache_find(g_pagecache, &key.cache_lru_node);
    if (found == NULL) {
        return NULL;
    }

    return container_of(found, struct page, cache_lru_node);
}

static struct page *pagecache_alloc_page(struct address_space *mapping, pgoff_t index)
{
    struct page *page = alloc_pages(0);

    if (!page) {
        printk("%s\n", "pagecache: alloc page failed");
        return NULL;
    }

    page->refcount = 0;
    spin_lock_init(&page->lock);
    page->mapping = mapping;
    page->index = index;
    page->private = NULL;
    page->dirty_blocks = 0;
    lru_node_reset(&page->cache_lru_node);
    memset(pagecache_data(page), 0, PAGE_SIZE);
    return page;
}

static int pagecache_try_reclaim_one(struct page *page)
{
    ASSERT(page != NULL, "pagecache: invalid reclaim page");

    lock_page(page);
    if (page->refcount != 0 || PageWriteback(page) ||
        (PageDirty(page) && !pagecache_can_writeback(page))) {
        unlock_page(page);
        return -1;
    }
    unlock_page(page);

    return lru_cache_remove(g_pagecache, &page->cache_lru_node);
}

int pagecache_init(void)
{
    g_pagecache = lru_cache_create(PAGECACHE_BUCKET_HINT, &pagecache_lru_ops, &pagecache_hash_ops);
    if (!g_pagecache) {
        printk("%s\n", "pagecache: init failed");
        return -1;
    }
    return 0;
}

void pagecache_destroy(void)
{
    if (g_pagecache != NULL) {
        lru_cache_destroy(g_pagecache);
        g_pagecache = NULL;
    }
}

struct page *pagecache_get_page(struct address_space *mapping, pgoff_t index, u32 flags)
{
    struct page *page = NULL;
    int ret = 0;

    if (g_pagecache == NULL) {
        return ERR_PTR(-EINVAL);
    }

    if (mapping == NULL) {
        return ERR_PTR(-EINVAL);
    }

    page = pagecache_lookup_nolock(mapping, index);
    if (page != NULL) {
        lock_page(page);
        page->refcount++;
        unlock_page(page);
        return page;
    }

    if ((flags & FGP_CREAT) == 0) {
        return NULL;
    }

    page = pagecache_alloc_page(mapping, index);
    if (page == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    ret = lru_cache_add(g_pagecache, &page->cache_lru_node);
    if (ret < 0) {
        free_pages(page);
        return ERR_PTR(-ENOMEM);
    }

    lock_page(page);
    page->refcount = 1;
    unlock_page(page);

    spin_lock(&mapping->lock);
    mapping->nrpages++;
    spin_unlock(&mapping->lock);

    if (g_pagecache->node_count > PAGECACHE_MAX_PAGES) {
        pagecache_reclaim_pages(g_pagecache->node_count - PAGECACHE_MAX_PAGES);
    }

    return page;
}

void pagecache_put_page(struct page *page)
{
    if (page == NULL) {
        return;
    }

    lock_page(page);
    if (page->refcount > 0) {
        page->refcount--;
    }
    unlock_page(page);

    if (g_pagecache != NULL) {
        lru_cache_touch(g_pagecache, &page->cache_lru_node);
    }
}

int pagecache_read_page(struct address_space *mapping, pgoff_t index,
                        bool allow_readahead, struct page **page_out)
{
    struct page *page = NULL;
    struct page *pages[PAGECACHE_RA_MAX_PAGES];
    struct page *batch[PAGECACHE_RA_MAX_PAGES];
    u32 nr_pages = 1;
    u32 nr_batch = 0;
    int ret = 0;

    if (mapping == NULL) {
        return -EINVAL;
    }
    if (page_out == NULL) {
        return -EINVAL;
    }
    *page_out = NULL;
    if (mapping->host == NULL) {
        return -EINVAL;
    }
    if (mapping->host->i_size == 0 ||
        index > (mapping->host->i_size - 1) / PAGE_SIZE) {
        return -ENOENT;
    }

    page = pagecache_get_page(mapping, index, FGP_CREAT);
    if (IS_ERR(page)) {
        return PTR_ERR(page);
    }
    if (page == NULL) {
        return -ENOENT;
    }

    lock_page(page);
    if (PageUptodate(page)) {
        unlock_page(page);
        *page_out = page;
        return 0;
    }
    unlock_page(page);

    if (!mapping->a_ops || !mapping->a_ops->readpage) {
        pagecache_put_page(page);
        return -EINVAL;
    }

    pages[0] = page;
    // 先取得所有引用再加页锁，避免分配时回收缓存又锁住当前页，导致死锁
    if (allow_readahead && mapping->a_ops->readpages) {
        u64 size = mapping->host->i_size;
        pgoff_t last = size ? (size - 1) / PAGE_SIZE : 0;

        for (u32 i = 1; size && index <= last && i < PAGECACHE_RA_MAX_PAGES && i <= last - index; i++) {
            struct page *next = pagecache_get_page(mapping, index + i, FGP_CREAT);
            if (IS_ERR(next) || !next) {
                break;
            }
            pages[nr_pages++] = next;
        }
    }

    for (u32 i = 0; i < nr_pages; i++)
        lock_page(pages[i]);

    // 取得额外页面期间，当前页可能已被其他读取填好
    if (PageUptodate(page))
        goto out_unlock;
    if (PageDirty(page) || PageWriteback(page)) {
        ret = -EIO;
        goto out_unlock;
    }

    for (u32 i = 0; i < nr_pages; i++) {
        if (!PageUptodate(pages[i]) && !PageDirty(pages[i]) &&!PageWriteback(pages[i])) {
            batch[nr_batch++] = pages[i];}
    }
    if (nr_batch > 1) {
        ret = mapping->a_ops->readpages(batch, nr_batch);
        if (ret == 0) {
            for (u32 i = 0; i < nr_batch; i++)
                SetPageUptodate(batch[i]);
        }
    }
    
    // 批量失败只重读调用者需要的当前页
    if (!PageUptodate(page)) {
        ret = mapping->a_ops->readpage(page);
        if (ret == 0)
            SetPageUptodate(page);
    }

out_unlock:
    for (u32 i = nr_pages; i > 0; i--)
        unlock_page(pages[i - 1]);
    for (u32 i = 1; i < nr_pages; i++)
        pagecache_put_page(pages[i]);

    if (ret < 0) {
        pagecache_put_page(page);
        return ret;
    }

    *page_out = page;
    return 0;
}

int pagecache_write_page(struct page *page)
{
    if (page == NULL) {
        return -EINVAL;
    }
    return pagecache_sync_page(&page->cache_lru_node);
}

int pagecache_sync_mapping(struct address_space *mapping)
{
    struct list_head *pos = NULL;
    struct list_head *next = NULL;
    int ret = 0;

    if (g_pagecache == NULL) {
        return -EINVAL;
    }
    if (mapping == NULL) {
        return -EINVAL;
    }

    pos = g_pagecache->lhead.next;
    while (pos != &g_pagecache->lhead) {
        struct page *page = container_of(pos, struct page, cache_lru_node.lnode);
        next = pos->next;
        if (page->mapping == mapping && PageDirty(page)) {
            ret = pagecache_write_page(page);
            if (ret < 0) {
                return ret;
            }
        }
        pos = next;
    }

    return 0;
}

int pagecache_invalidate_mapping(struct address_space *mapping)
{
    struct list_head *pos = NULL;
    struct list_head *next = NULL;

    if (g_pagecache == NULL) {
        return -EINVAL;
    }
    if (mapping == NULL) {
        return -EINVAL;
    }

    pos = g_pagecache->lhead.next;
    while (pos != &g_pagecache->lhead) {
        struct page *page = container_of(pos, struct page, cache_lru_node.lnode);
        next = pos->next;
        if (page->mapping == mapping) {
            lru_cache_remove(g_pagecache, &page->cache_lru_node);
        }
        pos = next;
    }

    return 0;
}

int pagecache_reclaim_pages(size_t nr_to_scan)
{
    struct list_head *pos = NULL;
    size_t reclaimed = 0;

    if (g_pagecache == NULL) {
        return -EINVAL;
    }

    pos = g_pagecache->lhead.prev;
    while (pos != &g_pagecache->lhead && reclaimed < nr_to_scan) {
        struct page *page = container_of(pos, struct page, cache_lru_node.lnode);
        struct list_head *prev = pos->prev;

        if (pagecache_try_reclaim_one(page) == 0) {
            reclaimed++;
        }
        pos = prev;
    }

    return (int)reclaimed;
}
