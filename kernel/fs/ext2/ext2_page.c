/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_page.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 19:39:53
 * @LastEditTime: 2025-09-02 18:26:25
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "ext2_block.h"
#include "block_adapter.h"

int64_t ext2_readpage(vfs_page_t *page) 
{
    vfs_inode_t *inode = page->inode;
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
            // 稀疏文件的空洞 -> 填零
            memset(kaddr + i * block_size, 0, block_size);
        } 
        else 
        {
            // 读一个块到内存
            block_adapter_read(inode->i_sb->adap, kaddr + i * block_size, phys_block, 1);
        }
    }

    page->uptodate = 1;  // 标记已加载
    return 0;
}


int64_t ext2_writepage(vfs_page_t *page) 
{
    vfs_inode_t *inode = page->inode;
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
            phys_block = ext2_alloc_bno(inode->i_sb,ext2_select_block_group(inode->i_sb));
            if (!phys_block)
                return -1;
        }

        // 2. 写磁盘
        void *buf = kaddr  + i * block_size ;
        block_adapter_write(inode->i_sb->adap, kaddr + i * block_size, phys_block, 1);
    }
    page->dirty = false;  
    return 0;
}


vfs_aops_t ext2_aops = 
{
    .readpage = ext2_readpage,
    .writepage = ext2_writepage,
};