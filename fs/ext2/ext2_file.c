/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/fs/ext2/ext2_file.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-04 18:30:45
 * @LastEditTime: 2025-10-22 23:01:46
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "pcache.h"
#include "icache.h"
#include "check.h"
#include "string.h"
#include "ext2_cache.h"
#include "ext2_super.h"


// int ext2_file_open(struct inode *inode, struct file *file)
// {
//     CHECK(inode != NULL && file != NULL, "ext2_file_open: inode or file is NULL", return -1;);
//     CHECK(inode->i_mapping != NULL, "ext2_file_open: inode->i_mapping is NULL", return -1;);
//     CHECK(inode->i_mapping->a_ops != NULL, "ext2_file_open: inode->i_mapping->a_ops is NULL", return -1;);
    
//     return 0;
// }

ssize_t ext2_file_read(struct inode *inode, void *buf, size_t size, loff_t *offset)
{
    CHECK(inode != NULL && buf != NULL, "ext2_read: inode or buf is NULL", return -1;);
    CHECK(inode->i_mapping != NULL, "ext2_read: inode->i_mapping is NULL", return -1;);
    CHECK(inode->i_mapping->a_ops != NULL, "ext2_read: inode->i_mapping->a_ops is NULL", return -1;);

    ssize_t bytes_read = 0;
    pgoff_t start_page = (*offset) / VFS_PAGE_SIZE;
    pgoff_t end_page = ((*offset) + size - 1) / VFS_PAGE_SIZE;
    uint32_t page_offset = 0;
    uint32_t bytes_to_copy = 0;

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
            if(size < VFS_PAGE_SIZE - page_offset)
            {
                bytes_to_copy = size;
            }
            else
            {
                bytes_to_copy = VFS_PAGE_SIZE - page_offset;
            }
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

        memcpy((uint8_t *)buf + (i - start_page) * VFS_PAGE_SIZE + page_offset, page->data + page_offset, bytes_to_copy);
        pput(page);
        bytes_read += bytes_to_copy;
    }
    *offset += bytes_read;
    return bytes_read;
}

ssize_t ext2_file_write(struct inode *inode, const void *buf, size_t size, loff_t *offset)
{
    CHECK(inode != NULL && buf != NULL, "ext2_read: inode or buf is NULL", return -1;);
    CHECK(inode->i_mapping != NULL, "ext2_read: inode->i_mapping is NULL", return -1;);
    CHECK(inode->i_mapping->a_ops != NULL, "ext2_read: inode->i_mapping->a_ops is NULL", return -1;);

    ssize_t bytes_write = 0;
    pgoff_t start_page = (*offset) / VFS_PAGE_SIZE;
    pgoff_t end_page = ((*offset) + size - 1) / VFS_PAGE_SIZE;
    uint32_t page_offset = 0;
    uint32_t bytes_to_copy = 0;

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
            if(size < VFS_PAGE_SIZE - page_offset)
            {
                bytes_to_copy = size;
            }
            else
            {
                bytes_to_copy = VFS_PAGE_SIZE - page_offset;
            }
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

        memcpy(page->data + page_offset, (uint8_t *)buf + (i - start_page) * VFS_PAGE_SIZE + page_offset, bytes_to_copy);
        page->dirty = true;
        pput(page);
        pcache_sync(page); // 同步page缓存
        bytes_write += bytes_to_copy;
    }
    *offset += bytes_write;
    
    inode->i_size = *offset > inode->i_size ? *offset : inode->i_size;
    inode->dirty = true;
    icache_sync(inode);

    inode->i_sb->s_ops->sync_fs(inode->i_sb); // 同步文件系统

    return bytes_write;
}


struct file_ops ext2_file_ops = {
    .open = NULL,
    .read = ext2_file_read,
    .write = ext2_file_write,
};