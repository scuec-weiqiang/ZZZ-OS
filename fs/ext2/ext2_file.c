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

        memcpy((u8 *)page_buf + page_offset,
               buf + bytes_written,
               bytes_to_copy);

        SetPageUptodate(page);
        SetPageDirty(page);

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
    pgoff_t start_page = (*ppos) / PAGE_SIZE;
    pgoff_t end_page = ((*ppos) + size - 1) / PAGE_SIZE;
    u32 page_offset = 0;
    u32 bytes_to_copy = 0;

    if (size == 0) {
        return 0;
    } else if (*ppos >= fp->f_inode->i_size) {
        return 0; // 已经到达文件末尾
    } else if ((*ppos + size) > fp->f_inode->i_size) {
        size = fp->f_inode->i_size - *ppos; // 调整size以不超过文件大小
    }

    for (pgoff_t i = start_page; i <= end_page; ++i)
    {
        struct page *page = ext2_get_page(fp->f_inode, i);
        
        if (IS_ERR(page)) {
            return PTR_ERR(page); // 读取页面失败
        }
        void *page_buf = page_address(page);
        if(i == start_page) {
            page_offset = (*ppos) % PAGE_SIZE;
            if(size < PAGE_SIZE - page_offset) {
                bytes_to_copy = size;
            } else {
                bytes_to_copy = PAGE_SIZE - page_offset;
            }
        } else if(i == end_page) {
            page_offset = 0;
            bytes_to_copy = (((*ppos) + size - 1) % PAGE_SIZE) + 1;
        } else {
            page_offset = 0;
            bytes_to_copy = PAGE_SIZE;
        }

        memcpy(buf + bytes_read, page_buf + page_offset, bytes_to_copy);
        ext2_put_page(page);
        bytes_read += bytes_to_copy;
    }
    *ppos += bytes_read;
    return bytes_read;
}

extern off_t generic_file_lseek(struct file *file, off_t offset, int whence);
const struct file_operations ext2_file_operations = {
    .open = ext2_file_open,
    .write = ext2_file_write,
    .read = ext2_file_read,
    .lseek = generic_file_lseek,
};
