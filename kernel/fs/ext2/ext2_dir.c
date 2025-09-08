/** 
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_dir.c
 * * @Description: * @Author: scuec_weiqiang scuec_weiqiang@qq.com 
 * @LastEditTime: 2025-09-07 23:12:31
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * * @Copyright : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025. */

#include "vfs_types.h"
#include "vfs_icache.h" 
#include "vfs_pcache.h"
#include "vfs_dcache.h" 
#include "ext2_cache.h" 
#include "ext2_fs.h"
#include "ext2_block.h" 
#include "ext2_super.h" 
#include "ext2_inode.h"

#include "check.h"
#include "list.h"
#include "page_alloc.h" 
#include "string.h"

#define EXT2_ENTRY_LEN(name_len) (((sizeof(ext2_dir_entry_2_t) + (name_len) + 3) / 4) * 4) 

static uint32_t ext2_mode_to_entry_type(uint32_t i_mode)
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

static int64_t ext2_init_dot_entries(vfs_inode_t *i_parent, uint32_t parent_inode)
{
    CHECK(i_parent != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = i_parent->i_sb;
    vfs_page_t *page = vfs_pget(i_parent, 0);

    ext2_dir_entry_2_t *dot = (ext2_dir_entry_2_t *)page->data;
    dot->inode = i_parent->i_ino;
    dot->name_len = 1;
    dot->file_type = EXT2_FT_DIR;
    dot->rec_len = EXT2_ENTRY_LEN(1);
    memcpy(dot->name, ".", 1);

    ext2_dir_entry_2_t *dotdot = (ext2_dir_entry_2_t *)(page->data + dot->rec_len);
    dotdot->inode = parent_inode;
    dotdot->name_len = 2;
    dotdot->file_type = EXT2_FT_DIR;
    dotdot->rec_len = vfs_sb->s_block_size - dot->rec_len;
    memcpy(dotdot->name, "..", 2);

    page->dirty = true;
    vfs_pput(page);
    
    return 0;
} 

/** 
* @brief 从虚拟文件系统（VFS）中获取指定文件或目录的dentry信息 
* 该函数在指定的父目录（parent_dentry）下查找名为name的文件或目录，并将找到的dentry信息复制到dentry_ret中。 
* 如果在父目录的子目录链表中找到了同名文件或目录，则直接返回对应的inode索引。 
* 如果在子目录链表中未找到，则从磁盘上读取目录项，并在遍历所有相关的block后返回找到的inode索引。 
* @param vfs_sb VFS超级块指针，包含文件系统的全局信息 
* @param i_parent 父目录的dentry指针
* @param name 要查找的文件或目录名 
* @param dentry_ret 用于存储找到的dentry信息的指针 
* @return 如果成功找到文件或目录，则返回对应的inode索引；如果未找到，则返回-1。 
*/
vfs_inode_t *ext2_lookup(vfs_inode_t *i_parent, const char *name)
{
    CHECK(i_parent != NULL, "", return -1;);
    CHECK(name != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = i_parent->i_sb;

    uint64_t page_num = (i_parent->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    vfs_inode_t *inode_ret = NULL; 
    size_t name_len = (size_t)strlen(name);

    for (uint64_t i = 0; i < page_num; i++)
    {
        vfs_page_t *page = vfs_pget(i_parent, i);
        uint32_t offset = 0; 
        ext2_dir_entry_2_t *entry = (ext2_dir_entry_2_t *)page->data; 
        // 开始对这个block中的目录项进行遍历
        while (offset < VFS_PAGE_SIZE)
        {
            entry = (ext2_dir_entry_2_t *)(page->data + offset);
            if (entry->inode != 0 && entry->name_len == name_len&& strncmp(entry->name, name, name_len) == 0)
            {
                inode_ret = vfs_iget(vfs_sb, entry->inode);
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


int64_t ext2_init_new_inode(vfs_inode_t *inode, uint32_t i_mode)
{
    CHECK(inode != NULL && inode->i_private != NULL, "", return -1;);
    CHECK(inode->i_sb != NULL && inode->i_sb->s_private != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = inode->i_sb;
    ext2_inode_t *new_inode = (ext2_inode_t*)inode->i_private;

    switch(EXT2_GET_TYPE(i_mode))
    {
        case EXT2_S_IFDIR:
            int64_t new_block_idx_ret = ext2_alloc_bno(vfs_sb,ext2_select_block_group(vfs_sb)); // 分配新的块
            CHECK(new_block_idx_ret >= 0, "", return -1;);       // 检查分配块是否成功
            new_inode->i_block[0] = (uint64_t)new_block_idx_ret; // 更新块索引
            new_inode->i_blocks += vfs_sb->s_block_size / 512;
            new_inode->i_size = vfs_sb->s_block_size;
            new_inode->i_links_count = 2; // . 和 ..
            break;
        case EXT2_S_IFREG:
            new_inode->i_size = 0;
            new_inode->i_block[0] = 0;
            new_inode->i_blocks = 0;
        default:
            new_inode->i_block[0] = 0;
            new_inode->i_blocks = 0;
            break;
    }
    uint32_t current_time = get_current_unix_timestamp(UTC8);
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

int64_t ext2_init_new_entry(ext2_dir_entry_2_t *new_entry, const char *name, uint32_t inode_idx, uint32_t i_mode) 
{
    new_entry->inode = inode_idx; // 设置新目录项指向的inode索引
    new_entry->name_len = strlen(name); // 设置新目录项的名称长度 
    new_entry->file_type = ext2_mode_to_entry_type(i_mode); // 设置entry文件类型
    memcpy(new_entry->name, name, new_entry->name_len); // 设置新目录项的名称 
}

void print_ext2_inode_member(vfs_superblock_t *vfs_sb, ext2_inode_t *inode)
{
    ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    printf("i_mode: %x\n", inode->i_mode);
    printf("i_uid: %d\n", inode->i_uid);
    printf("i_size: %du\n", inode->i_size);
    printf("i_atime: %du\n", inode->i_atime);
    printf("i_ctime: %du\n", inode->i_ctime);
    printf("i_mtime: %du\n", inode->i_mtime);
    printf("i_dtime: %du\n", inode->i_dtime);
    printf("i_gid: %d\n", inode->i_gid);
    printf("i_links_count: %d\n", inode->i_links_count);
    printf("i_blocks: %du\n", inode->i_blocks);
    printf("i_flags: %du\n", inode->i_flags);
    printf("i_block: ");
    for (int i = 0; i < 11; i++)
    {
        printf("%du ", inode->i_block[i]);
    }
    printf("\n");
    printf("i_generation: %du\n", inode->i_generation);
    printf("i_file_acl: %du\n", inode->i_file_acl);
    printf("i_dir_acl: %du\n", inode->i_dir_acl);
    printf("i_faddr: %du\n", inode->i_faddr);
    ext2_load_block_bitmap_cache(vfs_sb, 0);
    ext2_load_inode_bitmap_cache(vfs_sb, 0);
    bitmap_test_bit(fs_info->bbm_cache.bbm, inode->i_block[0]) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");
    bitmap_test_bit(fs_info->ibm_cache.ibm, 0xb) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");
}

// 辅助结构：查到的空槽信息
typedef struct {
    uint64_t page_index;      // 页号
    uint32_t offset;           // 在块内的偏移
    uint32_t prev_offset;      // 如果需要修改 prev_entry->rec_len
    uint32_t prev_real_len;   // prev_entry 的真实大小
    uint32_t free_len;        // 可用于新项的空间
    bool     found;            // 找到
    // bool     prev_is_empty_inode; // prev entry 的 inode==0
} dir_slot_t;
static int64_t ext2_find_free_slot_in_page(vfs_page_t *page, uint32_t need_len, dir_slot_t *out)
{
    uint8_t *page_buf = page->data;
    uint32_t offset = 0;
    ext2_dir_entry_2_t *entry = NULL;
    uint32_t entry_real_len = 0;

    while (offset < VFS_PAGE_SIZE)
    {
        entry = (ext2_dir_entry_2_t *)(page_buf + offset); // 获取当前目录项
        entry_real_len = EXT2_ENTRY_LEN(entry->name_len);// 计算当前目录项的真实大小,4字节对齐
        if (entry->inode == 0)
        {
            out->offset = offset;
            out->prev_offset = offset;
            out->prev_real_len = 0;
            out->free_len = entry->rec_len; // Fixed typo: private.out -> private->out
            out->found = true; // Fixed typo: private.out -> private->out
            out->page_index = page->index;
            // private->out->prev_is_empty_inode = true; // Fixed typo: private.out -> private->out
            return 0;
        }
        else if (entry->rec_len - entry_real_len >= need_len) // 如果空白位置足够大，可以在这里添加新目录项
        {
            out->offset = offset + entry_real_len; // 新目录项的位置
            out->prev_offset = offset;          // 需要修改的目录项位置
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
int64_t ext2_get_slot(vfs_inode_t *i_parent, size_t name_len, dir_slot_t *slot_out)
{
    CHECK(i_parent != NULL, "", return -1;);
    CHECK(name_len > 0, "", return -1;);

    vfs_superblock_t *vfs_sb = i_parent->i_sb;

    uint64_t page_num = (i_parent->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    for (uint64_t i = 0; i < page_num; i++)
    {
        vfs_page_t *page = vfs_pget(i_parent, i);
        uint32_t offset = 0; 
        uint32_t need_len = EXT2_ENTRY_LEN(name_len);
        ext2_find_free_slot_in_page(page,need_len,slot_out);
        if(slot_out->found == true)
        {
            vfs_pput(page);
            return 0;
        }
        vfs_pput(page);
    }
    return -1;
}

int64_t ext2_add_entry(vfs_inode_t *i_parent, vfs_dentry_t *dentry, dir_slot_t *slot,  uint32_t i_mode)
{
    CHECK(i_parent != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = i_parent->i_sb;

    vfs_page_t *page = vfs_pget(i_parent, slot->page_index); // 获取包含空槽的页
    ext2_dir_entry_2_t *new_entry = (ext2_dir_entry_2_t *)(page->data + slot->offset);
    ext2_dir_entry_2_t *prev_entry = (ext2_dir_entry_2_t *)(page->data + slot->prev_offset);
    if (slot->prev_real_len > 0) // 如果需要修改前一个目录项的rec_len
    {
        prev_entry->rec_len = slot->prev_real_len; // 调整前一个目录项的rec_len
    }
    new_entry->rec_len = slot->free_len; // 设置新目录项的rec_len
    ext2_init_new_entry(new_entry, dentry->name.name, dentry->d_inode->i_ino, i_mode); // 初始化新目录项
    
    page->dirty = true; // 标记页为脏
    vfs_pput(page); // 写回缓存

    ((ext2_inode_t*)i_parent->i_private)->i_links_count++; // 增加父目录链接计数 
    i_parent->dirty = true; // 标记父目录inode为脏
    i_parent->i_mtime.tv_sec = get_current_unix_timestamp(UTC8); // 更新修改时间
    ((ext2_fs_info_t*)(vfs_sb->s_private))->group_desc[ext2_ino_group(vfs_sb,dentry->d_inode->i_ino)].bg_used_dirs_count++; 


    return dentry->d_inode->i_ino; 
} 

int64_t ext2_mkdir(vfs_inode_t *i_parent, vfs_dentry_t *dentry, uint32_t i_mode) 
{ 
    CHECK(i_parent != NULL, "", return -1;);

    vfs_inode_t *new_inode = vfs_inew(i_parent->i_sb); // 创建新的inode 
    ext2_init_new_inode(new_inode, i_mode);// 初始化新的inode 
    ext2_init_dot_entries(new_inode, i_parent->i_ino); // 初始化 . 和 .. 目录项
    dentry->d_inode = new_inode; // 关联dentry和新inode
    vfs_iput(new_inode); // 写回缓存

    dir_slot_t slot = {0};
    ext2_get_slot(i_parent, dentry->name.len, &slot);
    ext2_add_entry(i_parent, dentry, &slot, i_mode);

    vfs_icache_sync(); // 同步inode缓存
    vfs_pcache_sync(); // 同步page缓存
    
    ext2_sync_cache(i_parent->i_sb); // 同步缓存 
    ext2_sync_super(i_parent->i_sb); // 同步superblock到磁盘 

    return 0; 
} 

int64_t ext2_remove_entry(vfs_inode_t *i_parent, vfs_dentry_t *dentry)
{

    return 0;
}

int64_t ext2_rmdir(vfs_inode_t *i_parent, vfs_dentry_t *dentry)
{
    
}



vfs_inode_ops_t ext2_inode_ops = 
{ 
    .lookup = ext2_lookup, 
    .mkdir = ext2_mkdir, 
};

