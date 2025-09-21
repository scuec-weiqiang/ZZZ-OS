/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_namei.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-12 22:55:01
 * @LastEditTime: 2025-09-13 00:27:19
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "ext2_types.h"
#include "ext2_super.h"
#include "ext2_dir.h"
#include "ext2_inode.h"
#include "ext2_cache.h"
#include "vfs_types.h"
#include "pcache.h"
#include "icache.h"
#include "check.h"

/**
* @brief 在ext2文件系统中查找指定目录项
*
* 在指定的目录中查找具有指定名称的目录项，并更新dentry结构中的inode指针。
*
* @param dir 指向目录的inode结构的指针
* @param dentry 指向要查找的目录项的dentry结构的指针
*
* @return 成功时返回0，失败时返回-1
*/
int ext2_lookup(struct inode *dir,struct dentry *dentry)
{
    CHECK(dir != NULL,"",return -1;);
    CHECK(dentry != NULL,"",return -1;);
    struct inode *d_inode = ext2_find(dir,dentry->name.name);
    if(d_inode==NULL)
    {
        return -1;
    }

    dentry->d_inode = d_inode;
    return 0;
}


static int ext2_add(struct inode *i_parent, struct dentry *dentry, u32 i_mode) 
{ 
    CHECK(i_parent != NULL, "", return -1;);

    struct inode *new_inode = inew(i_parent->i_sb); // 创建新的inode 
    ext2_init_new_inode(new_inode, i_mode);// 初始化新的inode 
    
    if(EXT2_GET_TYPE(i_mode) == EXT2_S_IFDIR)
    {
        ext2_init_dot_entries(new_inode, i_parent->i_ino); // 初始化 . 和 .. 目录项
    }
    dentry->d_inode = new_inode; // 关联dentry和新inode
    iput(new_inode); // 写回缓存

    dir_slot_t slot = {0};
    ext2_find_slot(i_parent, dentry->name.len, &slot);
    ext2_add_entry(i_parent, dentry, &slot, i_mode);

    
    i_parent->dirty = true; // 标记父目录inode为脏
    i_parent->i_mtime.tv_sec = get_current_unix_timestamp(UTC8); // 更新修改时间
    
    
    if(EXT2_GET_TYPE(i_mode) == EXT2_S_IFDIR)
    {
        ((struct ext2_inode*)i_parent->i_private)->i_links_count++; // 增加父目录链接计数 
        ((struct ext2_fs_info*)(i_parent->i_sb->s_private))->group_desc[ext2_ino_group(i_parent->i_sb,dentry->d_inode->i_ino)].bg_used_dirs_count++;
    }

    icache_sync(); // 同步inode缓存
    pcache_sync(); // 同步page缓存
    
    ext2_sync_cache(i_parent->i_sb); // 同步缓存 
    ext2_sync_super(i_parent->i_sb); // 同步superblock到磁盘 

    return 0; 
} 


int ext2_mkdir(struct inode *i_parent, struct dentry *dentry, u32 i_mode) 
{ 
    return ext2_add(i_parent, dentry, i_mode);
} 


int ext2_rmdir(struct inode *i_parent, struct dentry *dentry)
{
   return 0;
}

int ext2_creat(struct inode *i_parent, struct dentry *dentry, u32 i_mode) 
{ 
    return ext2_add(i_parent, dentry, i_mode);
} 

struct inode_ops ext2_inode_ops = 
{ 
    .lookup = ext2_lookup, 
    .mkdir = ext2_mkdir, 
};