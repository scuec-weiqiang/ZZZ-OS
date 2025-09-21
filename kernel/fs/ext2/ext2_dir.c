/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_dir.c
 * * @Description: * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-09-14 00:11:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * * @Copyright : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025. */

#include "vfs_types.h"
#include "icache.h"
#include "pcache.h"
#include "dcache.h"
#include "ext2_cache.h"
#include "ext2_types.h"
#include "ext2_block.h"
#include "ext2_super.h"
#include "ext2_inode.h"
#include "ext2_dir.h"

#include "check.h"
#include "list.h"
#include "malloc.h"
#include "string.h"

#define EXT2_ENTRY_LEN(name_len) (((sizeof(struct ext2_dir_entry_2) + (name_len) + 3) / 4) * 4)

static u32 ext2_mode_to_entry_type(u32 i_mode)
{
    switch (EXT2_GET_TYPE(i_mode))
    {
    case EXT2_S_IFDIR:
        return EXT2_FT_DIR;
    case EXT2_S_IFREG:
        return EXT2_FT_REG_FILE;
    default:
        return EXT2_FT_UNKNOWN;
    }
}

int ext2_init_dot_entries(struct inode *i_parent, u32 parent_inode)
{
    CHECK(i_parent != NULL, "", return -1;);

    struct superblock *vfs_sb = i_parent->i_sb;
    struct page *page = pget(i_parent, 0);

    struct ext2_dir_entry_2 *dot = (struct ext2_dir_entry_2 *)page->data;
    dot->inode = i_parent->i_ino;
    dot->name_len = 1;
    dot->file_type = EXT2_FT_DIR;
    dot->rec_len = EXT2_ENTRY_LEN(1);
    memcpy(dot->name, ".", 1);

    struct ext2_dir_entry_2 *dotdot = (struct ext2_dir_entry_2 *)(page->data + dot->rec_len);
    dotdot->inode = parent_inode;
    dotdot->name_len = 2;
    dotdot->file_type = EXT2_FT_DIR;
    dotdot->rec_len = vfs_sb->s_block_size - dot->rec_len;
    memcpy(dotdot->name, "..", 2);

    page->dirty = true;
    pput(page);

    return 0;
}

/**
 * @brief 从虚拟文件系统（VFS）中获取指定文件或目录的entry信息
 * 该函数在指定的父目录（parent_dentry）下查找名为name的文件或目录，并将找到的dentry信息复制到dentry_ret中。
 * 如果在父目录的子目录链表中找到了同名文件或目录，则直接返回对应的inode索引。
 * 如果在子目录链表中未找到，则从磁盘上读取目录项，并在遍历所有相关的block后返回找到的inode索引。
 * @param vfs_sb VFS超级块指针，包含文件系统的全局信息
 * @param i_parent 父目录的dentry指针
 * @param name 要查找的文件或目录名
 * @param dentry_ret 用于存储找到的dentry信息的指针
 * @return 如果成功找到文件或目录，则返回对应的inode索引；如果未找到，则返回-1。
 */
struct inode *ext2_find(struct inode *i_parent, const char *name)
{
    CHECK(i_parent != NULL, "", return NULL;);
    CHECK(name != NULL, "", return NULL;);

    struct superblock *vfs_sb = i_parent->i_sb;

    u64 page_num = (i_parent->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    struct inode *inode_ret = NULL;
    size_t name_len = (size_t)strlen(name);

    for (u64 i = 0; i < page_num; i++)
    {
        struct page *page = pget(i_parent, i);
        u32 offset = 0;
        struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)page->data;
        // 开始对这个block中的目录项进行遍历
        while (offset < VFS_PAGE_SIZE)
        {
            entry = (struct ext2_dir_entry_2 *)(page->data + offset);
            if (entry->name_len == name_len && (strncmp(entry->name, name, name_len) == 0))
            {
                if (entry->inode == 0)
                {
                    pput(page);
                    return NULL;
                }
                inode_ret = iget(vfs_sb, entry->inode);
                return inode_ret;
            }
            else
            {
                offset += entry->rec_len;
            }
        }
    }
    return NULL;
}

int ext2_init_new_inode(struct inode *inode, u32 i_mode)
{
    CHECK(inode != NULL && inode->i_private != NULL, "", return -1;);
    CHECK(inode->i_sb != NULL && inode->i_sb->s_private != NULL, "", return -1;);

    struct superblock *vfs_sb = inode->i_sb;
    struct ext2_inode *new_inode = (struct ext2_inode *)inode->i_private;

    switch (EXT2_GET_TYPE(i_mode))
    {
    case EXT2_S_IFDIR:
        int new_block_idx_ret = ext2_alloc_bno(vfs_sb); // 分配新的块
        CHECK(new_block_idx_ret >= 0, "", return -1;);  // 检查分配块是否成功
        new_inode->i_block[0] = (u64)new_block_idx_ret; // 更新块索引
        new_inode->i_blocks += vfs_sb->s_block_size / 512;
        new_inode->i_size = vfs_sb->s_block_size;
        new_inode->i_links_count = 2; // . 和 ..
        break;
    case EXT2_S_IFREG:
        new_inode->i_size = 0;
        new_inode->i_block[0] = 0;
        new_inode->i_blocks = 0;
        new_inode->i_links_count = 1;
    default:
        new_inode->i_block[0] = 0;
        new_inode->i_blocks = 0;
        break;
    }
    u32 current_time = get_current_unix_timestamp(UTC8);
    new_inode->i_atime = current_time;
    new_inode->i_ctime = current_time;
    new_inode->i_mtime = current_time;
    new_inode->i_dtime = 0;
    new_inode->i_mode = i_mode;
    new_inode->i_uid = 0; // root
    new_inode->i_gid = 0; // root

    // 同步公共字段
    inode->i_size = new_inode->i_size;
    inode->i_mode = new_inode->i_mode;
    inode->i_uid = new_inode->i_uid;
    inode->i_gid = new_inode->i_gid;
    inode->i_nlink = new_inode->i_links_count;
    inode->i_atime.tv_sec = new_inode->i_atime;
    inode->i_ctime.tv_sec = new_inode->i_ctime;
    inode->i_mtime.tv_sec = new_inode->i_mtime;

    inode->dirty = true; // 标记为脏，需要写回

    return 0;
}

int ext2_init_new_entry(struct ext2_dir_entry_2 *new_entry, const char *name, u32 inode_idx, u32 i_mode)
{
    new_entry->inode = inode_idx;                           // 设置新目录项指向的inode索引
    new_entry->name_len = strlen(name);                     // 设置新目录项的名称长度
    new_entry->file_type = ext2_mode_to_entry_type(i_mode); // 设置entry文件类型
    memcpy(new_entry->name, name, new_entry->name_len);     // 设置新目录项的名称
    return 0;
}

static int ext2_find_free_slot_in_page(struct page *page, u32 need_len, dir_slot_t *out)
{
    char *page_buf = page->data;
    u32 offset = 0;
    struct ext2_dir_entry_2 *entry = NULL;
    u32 entry_real_len = 0;

    while (offset < VFS_PAGE_SIZE)
    {
        entry = (struct ext2_dir_entry_2 *)(page_buf + offset); // 获取当前目录项
        entry_real_len = EXT2_ENTRY_LEN(entry->name_len);       // 计算当前目录项的真实大小,4字节对齐
        if (entry->inode == 0)
        {
            out->offset = offset;
            out->prev_offset = offset;
            out->prev_real_len = 0;
            out->free_len = entry->rec_len; // Fixed typo: private.out -> private->out
            out->found = true;              // Fixed typo: private.out -> private->out
            out->page_index = page->index;
            // private->out->prev_is_empty_inode = true; // Fixed typo: private.out -> private->out
            return 0;
        }
        else if (entry->rec_len - entry_real_len >= need_len) // 如果空白位置足够大，可以在这里添加新目录项
        {
            out->offset = offset + entry_real_len;           // 新目录项的位置
            out->prev_offset = offset;                       // 需要修改的目录项位置
            out->free_len = entry->rec_len - entry_real_len; // 可用空间大小
            out->found = true;
            out->prev_real_len = entry_real_len;
            out->page_index = page->index;
            return 0;
        }
        offset += entry->rec_len; // 移动到下一个目录项
    }
    return -1;
}

int ext2_find_slot(struct inode *i_parent, size_t name_len, dir_slot_t *slot_out)
{
    CHECK(i_parent != NULL, "", return -1;);
    CHECK(name_len > 0, "", return -1;);

    u64 page_num = (i_parent->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    for (u64 i = 0; i < page_num; i++)
    {
        struct page *page = pget(i_parent, i);
        u32 need_len = EXT2_ENTRY_LEN(name_len);
        ext2_find_free_slot_in_page(page, need_len, slot_out);
        if (slot_out->found == true)
        {
            pput(page);
            return 0;
        }
        pput(page);
    }
    return -1;
}

int ext2_add_entry(struct inode *i_parent, struct dentry *dentry, dir_slot_t *slot, u32 i_mode)
{
    CHECK(i_parent != NULL, "", return -1;);

    // struct superblock *vfs_sb = i_parent->i_sb;

    struct page *page = pget(i_parent, slot->page_index); // 获取包含空槽的页
    struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(page->data + slot->offset);
    struct ext2_dir_entry_2 *prev_entry = (struct ext2_dir_entry_2 *)(page->data + slot->prev_offset);
    if (slot->prev_real_len > 0) // 如果需要修改前一个目录项的rec_len
    {
        prev_entry->rec_len = slot->prev_real_len; // 调整前一个目录项的rec_len
    }
    new_entry->rec_len = slot->free_len;                                               // 设置新目录项的rec_len
    ext2_init_new_entry(new_entry, dentry->name.name, dentry->d_inode->i_ino, i_mode); // 初始化新目录项

    page->dirty = true; // 标记页为脏
    pput(page);         // 写回缓存

    return dentry->d_inode->i_ino;
}

int ext2_remove_entry(struct inode *i_parent, struct dentry *dentry)
{

    return 0;
}
