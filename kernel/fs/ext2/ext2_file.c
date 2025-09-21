#include "vfs_types.h"
#include "pcache.h"
#include "check.h"
#include "string.h"

ssize_t ext2_file_read(struct inode *inode, void *buf, size_t size, loff_t *offset)
{
    CHECK(inode != NULL && buf != NULL, "ext2_read: inode or buf is NULL", return -1;);
    CHECK(inode->i_mapping != NULL, "ext2_read: inode->i_mapping is NULL", return -1;);
    CHECK(inode->i_mapping->a_ops != NULL, "ext2_read: inode->i_mapping->a_ops is NULL", return -1;);

    ssize_t bytes_read = 0;
    pgoff_t start_page = (*offset) / VFS_PAGE_SIZE;
    pgoff_t end_page = ((*offset) + size - 1) / VFS_PAGE_SIZE;
    u32 page_offset = 0;
    u32 bytes_to_copy = 0;

    for (pgoff_t i = start_page; i <= end_page; ++i)
    {
        struct page *page = pget(inode, i);
        if (page == NULL)
        {
            return -1; // 读取页面失败
        }

        if(i == start_page)
        {
            page_offset = (*offset) % VFS_PAGE_SIZE;
            bytes_to_copy = VFS_PAGE_SIZE - page_offset;
        }
        else if(i == end_page)
        {
            page_offset = 0;
            bytes_to_copy = (((*offset) + size - 1) % VFS_PAGE_SIZE) + 1;
        }
        else
        {
            page_offset = 0;
            bytes_to_copy = VFS_PAGE_SIZE;
        }

        memcpy((u8 *)buf + (i - start_page) * VFS_PAGE_SIZE + page_offset, page->data + page_offset, bytes_to_copy);
        pput(page);
        bytes_read += bytes_to_copy;
    }
    *offset += bytes_read;
    return bytes_read;
}

ssize_t ext2_file_write(struct inode *inode, const void *buf, size_t size, loff_t *offset)
{
    return 0;
}


struct file_ops ext2_file_ops = {
    .read = ext2_file_read,
    .write = ext2_file_write,
};