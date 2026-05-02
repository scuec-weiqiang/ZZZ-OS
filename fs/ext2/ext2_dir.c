#include <mm/page.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/pagecache.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/string.h>
#include "ext2_types.h"

extern int ext2_block_mapping(struct inode *inode, u32 index);

static int ext2_readpage(struct page *page) {
    struct inode *inode= page->mapping->host;
    struct blkdev *bdev = page->mapping->host->i_sb->s_bdev;
    u8 *page_buf = (u8*)page_address(page);

    u32 block_size = inode->i_sb->s_blocksize;
    u32 blocks_per_page = PAGE_SIZE / block_size;
    u32 first_file_block = page->index * blocks_per_page;

    u32 file_block = 0;
    u32 disk_block = 0;

    int ret = -EIO;

    for (int i = 0; i < blocks_per_page; i++) {
        file_block = first_file_block + i;
        disk_block = ext2_block_mapping(inode, file_block);

        if (disk_block == 0) {
            memset(page_buf + i * block_size, 0, block_size);
            continue;
        }

        u64 pos = (u64)disk_block * block_size;
        ret = blkdev_read(bdev, page_buf + i * block_size, block_size, pos);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

static int ext2_writepage(struct page *page) {
    return -EINVAL;
}


const struct address_space_operations ext2_aops = {
    .readpage = ext2_readpage,
    .writepage = ext2_writepage,
};

struct page *ext2_get_page(struct inode *inode, u32 index) {
    struct page *page = NULL;
    int ret = 0;

    if (inode == NULL) {
        return ERR_PTR(-EINVAL);
    }

    page = pagecache_get_page(inode->i_mapping, index, FGP_CREAT);
    if (IS_ERR(page)) {
        return page;
    }
    if (page == NULL) {
        return ERR_PTR(-ENOENT);
    }

    lock_page(page);
    if (!PageUptodate(page)) {
        if (inode->i_mapping->a_ops == NULL || inode->i_mapping->a_ops->readpage == NULL) {
            unlock_page(page);
            pagecache_put_page(page);
            return ERR_PTR(-EINVAL);
        }

        ret = inode->i_mapping->a_ops->readpage(page);
        if (ret == 0) {
            SetPageUptodate(page);
        }
    }
    unlock_page(page);

    if (ret < 0) {
        pagecache_put_page(page);
        return ERR_PTR(ret);
    }

    return page;
}

void ext2_put_page(struct page *page) {
    pagecache_put_page(page);
}

static u32 last_valid_byte(struct inode *inode, u32 page_nr) {
    u32 last_byte = inode->i_size;
    last_byte -= page_nr * PAGE_SIZE;
    if (last_byte > PAGE_SIZE) {
        last_byte = PAGE_SIZE;
    }

    return last_byte;
}

static struct ext2_dir_entry_2 *ext2_find_entry(struct inode *dir, struct qstr *child, struct page **out_page) {
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;
    u32 rec_len = EXT2_DIR_REC_LEN(child->len);
    struct page *page = NULL;
    char *buf = NULL;
    struct ext2_dir_entry_2 *child_entry = NULL;

    for (int i = 0; i < page_nr; i++) {
        page = ext2_get_page(dir, i);
        if (IS_ERR(page)) {
            return ERR_CAST(page);
        }
        buf = (char *)page_address(page);
        u32 valid_bytes_in_page = last_valid_byte(dir, i);
        char *end = (char*)(valid_bytes_in_page - rec_len);
        u32 offset = 0;
        while (offset < (u32)end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);
            if (entry->inode != 0 && entry->name_len == child->len &&
                memcmp(entry->name, child->name, child->len) == 0) {
                child_entry = entry;
                *out_page = page;
                goto found;
            }
            offset += entry->rec_len;
        }
    }

found:
    return child_entry;
}


u32 ext2_find(struct inode *dir, struct qstr *child) {
    struct page *page = NULL;
    struct ext2_dir_entry_2 *entry = ext2_find_entry(dir, child, &page);
    if (IS_ERR(entry)) {
        return 0;
    }
    if (entry == NULL) {
        return 0;
    }
    ext2_put_page(page);
    return entry->inode;
}

