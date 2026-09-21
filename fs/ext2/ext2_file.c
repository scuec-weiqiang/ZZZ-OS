#include <fs/types.h>
#include <fs/inode.h>
#include <fs/file.h>
#include <os/err.h>
#include <os/string.h>
#include <mm/page.h>
#include <fs/pagecache.h>
#include <os/printk.h>
#include "ext2_types.h"

int ext2_file_open(struct inode *inode, struct file *file) {
    return 0;
}

extern int ext2_block_mapping(struct inode *inode, u32 index);
extern int ext2_block_set_mapping(struct inode *inode, u32 index);

static int ext2_prepare_write(struct inode *inode, loff_t pos, size_t len) {
    u32 block_size = inode->i_sb->s_blocksize;
    u32 first_file_block;
    u32 last_file_block;
    u32 file_block;

    first_file_block = (u32)pos / block_size;
    last_file_block = ((u32)pos + len - 1) / block_size;

    for (file_block = first_file_block; file_block <= last_file_block; file_block++) {
        int disk_bno = ext2_block_set_mapping(inode, file_block);
        if (disk_bno < 0) {
            return disk_bno;
        }
            
    }

    return 0;
}

extern int ext2_write_inode(struct inode *inode);

static ssize_t ext2_file_write(struct file *fp, const char *buf, size_t size, loff_t *ppos) {
    struct inode *inode = fp->f_inode;
    ssize_t bytes_written = 0;
    loff_t pos = *ppos;
    int ret = 0;

    if (size == 0)
        return 0;

    while (bytes_written < size) {
        pgoff_t page_index;
        u32 page_offset;
        u32 bytes_to_copy;
        struct page *page;
        void *page_buf;

        page_index = pos / PAGE_SIZE;
        page_offset = pos % PAGE_SIZE;

        bytes_to_copy = PAGE_SIZE - page_offset;
        if (bytes_to_copy > size - bytes_written)
            bytes_to_copy = size - bytes_written;

        ret = ext2_prepare_write(inode, pos, bytes_to_copy);
        if (ret < 0)
            break;
        
        page = ext2_get_page(inode, page_index);
        if (IS_ERR(page)) {
            ret = PTR_ERR(page);
            break;
        }

        page_buf = page_address(page);

        lock_page(page);
        ret = pagecache_mark_dirty_range(page, page_offset, bytes_to_copy,
                                        inode->i_sb->s_blocksize);
        if (ret < 0) {
            unlock_page(page);
            ext2_put_page(page);
            break;
        }

        memcpy((u8 *)page_buf + page_offset,
               buf + bytes_written,
               bytes_to_copy);

        SetPageUptodate(page);
        unlock_page(page);

        ret = pagecache_write_page(page);

        ext2_put_page(page);

        if (ret < 0)
            break;

        pos += bytes_to_copy;
        bytes_written += bytes_to_copy;
    }

    if (bytes_written > 0) {
        *ppos += bytes_written;

        if (*ppos > inode->i_size)
            inode->i_size = *ppos;

        ext2_write_inode(inode);
        return bytes_written;
    }

    return ret;
}

static ssize_t ext2_file_read(struct file *fp, char *buf, size_t size, loff_t *ppos) {
    ssize_t bytes_read = 0;
    loff_t pos = *ppos;
    int ret = 0;

    if (pos < 0)
        return -EINVAL;

    if (size == 0 || pos >= fp->f_inode->i_size)
        return 0;

    if (size > fp->f_inode->i_size - pos)
        size = fp->f_inode->i_size - pos;

    struct file_ra_state *ra = &fp->f_ra;
    // 这次读写的起始位置是否和上次读写的结束位置相连, 如果是, 则说明是顺序读写, 否则说明是随机读写
    bool sequential = ra->valid && (pos == ra->prev_end);
    bool allow_readahead = sequential && !ra->disabled;

    while ((size_t)bytes_read < size) {
        struct page *page;
        size_t offset = pos % PAGE_SIZE;
        size_t length = PAGE_SIZE - offset;

        if (length > size - bytes_read)
            length = size - bytes_read;
        ret = pagecache_read_page(fp->f_inode->i_mapping,
                                  pos / PAGE_SIZE, allow_readahead, &page);
        if (ret < 0)
            break;
        memcpy(buf + bytes_read, (u8 *)page_address(page) + offset, length);
        ext2_put_page(page);
        bytes_read += length;
        pos += length;
        allow_readahead = !ra->disabled;
    }
    if (bytes_read > 0) {
        ra->prev_end = pos;
        ra->valid = true;
    }
    *ppos = pos;
    return bytes_read ? bytes_read : ret;
}

extern off_t generic_file_lseek(struct file *file, off_t offset, int whence);
const struct file_operations ext2_file_operations = {
    .open = ext2_file_open,
    .write = ext2_file_write,
    .read = ext2_file_read,
    .lseek = generic_file_lseek,
};
