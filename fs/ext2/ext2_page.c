/**
 * @FilePath: /ZZZ-OS/fs/ext2/ext2_page.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 19:39:53
 * @LastEditTime: 2025-10-06 21:26:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "ext2_types.h"
#include "ext2_block.h"
#include "block_adapter.h"
#include "string.h"

int ext2_readpage(struct page *page) 
{
    struct inode *inode = page->inode;
    page->under_io = true;  // 表示正在进行IO操作
    uint32_t page_index = page->index;  // 页号
    uint32_t block_size = inode->i_sb->s_block_size;
    uint32_t blocks_per_page = VFS_PAGE_SIZE / block_size;

    char *kaddr = page->data;  // 页的内存地址

    for (int i = 0; i < blocks_per_page; i++) 
    {
        uint32_t file_block = page_index * blocks_per_page + i;
        uint32_t phys_block = ext2_block_mapping(inode, file_block); // 文件逻辑块号 → 磁盘物理块号

        if (phys_block == 0) 
        {
            // phys_block = ext2_alloc_bno(inode->i_sb);
            // if (!phys_block) return -1;
            // ((struct ext2_inode*)inode->i_private)->i_block[file_block] = phys_block;
            // inode->dirty = true;
            // 新块缓存直接填零,没必要去读了
            memset(kaddr + i * block_size, 0, block_size);
            continue;
        } 
        
        // 读一个块到内存
        block_adapter_read(inode->i_sb->adap, kaddr + i * block_size, phys_block, 1);
    }
    page->under_io = false;  // 标记IO完成
    page->uptodate = 1;  // 标记已加载
    return 0;
}


int ext2_writepage(struct page *page) 
{
    struct inode *inode = page->inode;
    page->under_io = true;  // 表示正在进行IO操作
    uint32_t page_index = page->index;  // 页号
    uint32_t block_size = inode->i_sb->s_block_size;
    uint32_t blocks_per_page = VFS_PAGE_SIZE / block_size;

    char *kaddr = page->data;  // 页的内存地址

    for (int i = 0; i < blocks_per_page; i++) 
    {
        uint32_t file_block = page_index * blocks_per_page + i;
        uint32_t phys_block = ext2_block_mapping(inode, file_block); // 文件逻辑块号 → 磁盘物理块号

        if (phys_block == 0) 
        {
            // 说明文件还没分配这个块，需要分配
            phys_block = ext2_alloc_bno(inode->i_sb);
            if (!phys_block)
                return -1;
            ((struct ext2_inode*)inode->i_private)->i_block[file_block] = phys_block;
            inode->dirty = true;
        }

        // 2. 写磁盘
        block_adapter_write(inode->i_sb->adap, kaddr + i * block_size, phys_block, 1);
    }
    page->under_io = false;  // 标记IO完成
    page->dirty = false;  
    return 0;
}


struct aops ext2_aops = 
{
    .readpage = ext2_readpage,
    .writepage = ext2_writepage,
};