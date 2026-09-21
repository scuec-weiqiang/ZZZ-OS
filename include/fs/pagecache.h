#ifndef FS2_PAGECACHE_H
#define FS2_PAGECACHE_H

#include <fs/types.h>
#include <mm/page.h>
#include <mm/physmem.h>
#include <os/kva.h>
#include <os/errno.h>

#define FGP_CREAT   (1U << 0)
#define PAGECACHE_RA_MAX_PAGES 4U

int pagecache_init(void);
void pagecache_destroy(void);

struct page *pagecache_get_page(struct address_space *mapping, pgoff_t index, u32 flags);
void pagecache_put_page(struct page *page);
int pagecache_read_page(struct address_space *mapping, pgoff_t index,
                        bool allow_readahead, struct page **page_out);
int pagecache_write_page(struct page *page);
int pagecache_sync_mapping(struct address_space *mapping);
int pagecache_invalidate_mapping(struct address_space *mapping);
int pagecache_reclaim_pages(size_t nr_to_scan);

static inline void *pagecache_data(struct page *page)
{
    return (void *)KERNEL_VA(page_to_phys(page));
}

static inline void lock_page(struct page *page)
{
    if (page != NULL) {
        spin_lock(&page->lock);
        page_set_flag(page, PAGE_LOCKED);
    }
}

static inline void unlock_page(struct page *page)
{
    if (page != NULL) {
        page_clear_flag(page, PAGE_LOCKED);
        spin_unlock(&page->lock);
    }
}

static inline int PageUptodate(struct page *page)
{
    return page_test_flag(page, PAGE_UPTODATE);
}

static inline void SetPageUptodate(struct page *page)
{
    page_set_flag(page, PAGE_UPTODATE);
}

static inline void ClearPageUptodate(struct page *page)
{
    page_clear_flag(page, PAGE_UPTODATE);
}

static inline int PageDirty(struct page *page)
{
    return page_test_flag(page, PAGE_DIRTY);
}

static inline void SetPageDirty(struct page *page)
{
    page->dirty_blocks = 0;
    page_set_flag(page, PAGE_DIRTY);
}

static inline int pagecache_mark_dirty_range(struct page *page, u32 offset,
                                            u32 length, u32 block_size)
{
    u32 first;
    u32 last;

    if (!page || block_size < 512 || block_size > PAGE_SIZE ||
        PAGE_SIZE % block_size || PAGE_SIZE / block_size > 8 ||
        offset > PAGE_SIZE || length > PAGE_SIZE - offset)
        return -EINVAL;
    if (!length)
        return 0;

    /* 已经整页脏时，不能用局部范围缩小原来的写回范围。 */
    if (PageDirty(page) && !page->dirty_blocks)
        return 0;

    first = offset / block_size;
    last = (offset + length - 1) / block_size;
    for (u32 i = first; i <= last; i++)
        page->dirty_blocks |= 1U << i;
    page_set_flag(page, PAGE_DIRTY);
    return 0;
}

static inline void ClearPageDirty(struct page *page)
{
    page->dirty_blocks = 0;
    page_clear_flag(page, PAGE_DIRTY);
}

static inline int PageWriteback(struct page *page)
{
    return page_test_flag(page, PAGE_WRITEBACK);
}

static inline void SetPageWriteback(struct page *page)
{
    page_set_flag(page, PAGE_WRITEBACK);
}

static inline void ClearPageWriteback(struct page *page)
{
    page_clear_flag(page, PAGE_WRITEBACK);
}

#endif
