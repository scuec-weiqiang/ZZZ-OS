#ifndef FS2_PAGECACHE_H
#define FS2_PAGECACHE_H

#include <fs/types.h>
#include <mm/page.h>
#include <mm/physmem.h>
#include <os/kva.h>

#define FGP_CREAT   (1U << 0)

int pagecache_init(void);
void pagecache_destroy(void);

struct page *pagecache_get_page(struct address_space *mapping, pgoff_t index, uint32_t flags);
void pagecache_put_page(struct page *page);
int pagecache_read_page(struct address_space *mapping, pgoff_t index, struct page **page_out);
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
    page_set_flag(page, PAGE_DIRTY);
}

static inline void ClearPageDirty(struct page *page)
{
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
