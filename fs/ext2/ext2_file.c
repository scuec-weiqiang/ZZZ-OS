#include <fs/types.h>
#include <fs/inode.h>
#include <fs/file.h>
#include <os/err.h>
#include <os/string.h>
#include <mm/page.h>

#include "ext2_types.h"


ssize_t ext2_file_read(struct file *fp, char *buf, size_t size, loff_t *ppos) {
    ssize_t bytes_read = 0;
    pgoff_t start_page = (*ppos) / PAGE_SIZE;
    pgoff_t end_page = ((*ppos) + size - 1) / PAGE_SIZE;
    uint32_t page_offset = 0;
    uint32_t bytes_to_copy = 0;

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

const struct file_operations ext2_file_operations = {
    .read = ext2_file_read,
};
