/**
 * @FilePath: /ZZZ-OS/fs/ext2/ext2_inode.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-15 15:04:59
 * @LastEditTime: 2025-10-09 23:17:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "ext2_inode.h"
#include "vfs_types.h"
#include "ext2_types.h"
#include "icache.h"
#include "malloc.h"
#include "check.h"
#include "string.h"
#include "spinlock.h"
#include "ext2_cache.h"
#include "time.h"
#include "ext2_block.h"
#include "ext2_page.h"
#include "ext2_namei.h"
#include "ext2_file.h"

uint64_t ext2_ino_group(struct superblock *vfs_sb,uint64_t ino)
{
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private;
    return (ino) / fs_info->s_inodes_per_group;
}

/**
* @brief 在ext2文件系统中分配一个新的inode编号
*
* 该函数在ext2文件系统中分配一个新的inode编号，并返回该编号。
*
* @param vfs_sb VFS超级块指针
*
* @return 成功时返回分配的inode编号，失败时返回-1
*/
int ext2_alloc_ino(struct superblock *vfs_sb)
{
    CHECK(vfs_sb !=NULL && vfs_sb->s_private != NULL, "", return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private;

    uint64_t group = ext2_select_inode_group(vfs_sb);

    // 加载缓存
    ext2_load_inode_bitmap_cache(vfs_sb,group);
    int free_idx = bitmap_scan_0(fs_info->ibm_cache.ibm);
    CHECK(free_idx >= 0, "", return -1;);

    bitmap_set_bit(fs_info->ibm_cache.ibm,free_idx);
    fs_info->ibm_cache.dirty = true;
    fs_info->s_free_inodes_count--;
    fs_info->super->s_free_inodes_count--;
    fs_info->group_desc[group].bg_free_inodes_count--;

    free_idx += group * fs_info->s_inodes_per_group + 1; // inode编号从1开始

    fs_info->ibm_cache.dirty = true; // 标记inode table缓存为脏
    
    return free_idx;
}

int ext2_release_ino(struct superblock *vfs_sb, uint64_t ino)
{
    CHECK(vfs_sb != NULL && vfs_sb->s_private != NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private;
    CHECK(ino <= fs_info->super->s_inodes_count,"",return -1;);

    uint64_t group = ext2_ino_group(vfs_sb,ino);
    uint64_t index = ino % fs_info->s_inodes_per_group - 1;

    // 清除位图缓存，并更新inode总数和空闲数
    ext2_load_inode_bitmap_cache(vfs_sb,group);
    bitmap_clear_bit(fs_info->ibm_cache.ibm,index);
    fs_info->ibm_cache.dirty = true;
    fs_info->s_free_inodes_count++;
    fs_info->super->s_free_inodes_count++;
    fs_info->group_desc[group].bg_free_inodes_count++;

    return 0;
}

void* ext2_create_inode(struct inode *inode)
{
    CHECK(inode != NULL && inode->i_sb != NULL,"",return NULL;);

    struct ext2_inode *new_private_inode = malloc(sizeof(struct ext2_inode));
    memset(new_private_inode,0,sizeof(struct ext2_inode));
    
    // 同步公共字段
    new_private_inode->i_size = inode->i_size;
    new_private_inode->i_mode = inode->i_mode;
    new_private_inode->i_uid = inode->i_uid;
    new_private_inode->i_gid = inode->i_gid;
    new_private_inode->i_links_count = inode->i_nlink;
    new_private_inode->i_atime = inode->i_atime.tv_sec;
    new_private_inode->i_ctime = inode->i_ctime.tv_sec;
    new_private_inode->i_mtime = inode->i_mtime.tv_sec;

    inode->i_mapping->a_ops = &ext2_aops;
    inode->i_ops = &ext2_inode_ops;
    inode->f_ops = &ext2_file_ops;

    return new_private_inode;
}

int ext2_new_inode(struct inode *inode)
{
    CHECK(inode != NULL && inode->i_sb != NULL && inode->i_sb->s_private != NULL,"",return -1;);
    struct superblock *vfs_sb = inode->i_sb;

    inode->i_private = ext2_create_inode(inode);
    CHECK(inode->i_private != NULL,"",return -1;);
    int ino = ext2_alloc_ino(vfs_sb);
    CHECK(ino > 0,"",free(inode->i_private);return -1;);
    inode->i_ino = ino;
    return ino;
}

/**
* @brief 从ext2文件系统中读取inode信息
*
* 从ext2文件系统中读取指定inode编号的inode信息，并将其填充到vfs_inode_t结构体中。
*
* @param vfs_sb VFS超级块指针
* @param ino inode编号
* @param inode_ret VFS inode结构体指针，用于存储读取的inode信息
*
* @return 成功时返回inode编号，失败时返回-1
*/
int ext2_read_inode(struct inode *inode_ret)
{
    CHECK(inode_ret != NULL, "", return -1;);
    CHECK(inode_ret->i_sb != NULL && inode_ret->i_sb->s_private != NULL, "", return -1;);
    CHECK(inode_ret->i_private != NULL,"",return -1;);

    struct superblock *vfs_sb = inode_ret->i_sb;
    struct ext2_fs_info *fs_info = (struct ext2_fs_info *)vfs_sb->s_private;
    uint32_t group = ext2_ino_group(vfs_sb,inode_ret->i_ino);
    uint32_t index = inode_ret->i_ino % fs_info->s_inodes_per_group -1;

    // 确保 inode table 已经在内存中
    int ret = ext2_load_inode_table_cache(vfs_sb, group);
    CHECK(ret >= 0, "", return -1;);
  
    // 拿到 ext2 的 inode 指针
    struct ext2_inode *raw_inode = &fs_info->it_cache.it[index];

    // 复制 FS 私有 inode 数据内存
    memcpy(inode_ret->i_private, raw_inode, sizeof(struct ext2_inode));

    // 同步公共字段
    inode_ret->i_size = raw_inode->i_size;  
    inode_ret->i_mode = raw_inode->i_mode;
    inode_ret->i_uid = raw_inode->i_uid;
    inode_ret->i_gid = raw_inode->i_gid;
    inode_ret->i_nlink = raw_inode->i_links_count;
    inode_ret->i_atime.tv_sec = raw_inode->i_atime;
    inode_ret->i_ctime.tv_sec = raw_inode->i_ctime;
    inode_ret->i_mtime.tv_sec = raw_inode->i_mtime;

    if(S_ISCHR(inode_ret->i_mode))
    {
        inode_ret->i_rdev = raw_inode->i_block[0];
    }
  
    return inode_ret->i_ino;
}

int ext2_write_inode(struct inode *inode)
{
    CHECK(inode != NULL, "", return -1;);
    CHECK(inode->i_sb != NULL && inode->i_sb->s_private != NULL, "", return -1;);
    CHECK(inode->i_private != NULL,"",return -1;);

    struct superblock *vfs_sb = inode->i_sb;
    struct ext2_inode *ext2_inode = (struct ext2_inode *)inode->i_private;

    // 同步公共字段
    ext2_inode->i_size = inode->i_size;  
    ext2_inode->i_mode = inode->i_mode;
    ext2_inode->i_uid = inode->i_uid;
    ext2_inode->i_gid = inode->i_gid;
    ext2_inode->i_links_count = inode->i_nlink;
    ext2_inode->i_atime = inode->i_atime.tv_sec;
    ext2_inode->i_ctime = inode->i_ctime.tv_sec;
    ext2_inode->i_mtime = inode->i_mtime.tv_sec;

    struct ext2_fs_info *fs_info = (struct ext2_fs_info *)vfs_sb->s_private;
    uint32_t group = ext2_ino_group(vfs_sb,inode->i_ino);
    uint32_t index = inode->i_ino % fs_info->s_inodes_per_group - 1;

    // 确保 inode table 已经在内存中
    int ret = ext2_load_inode_table_cache(vfs_sb, group);
    CHECK(ret >= 0, "", return -1;);

    // 更新缓存中的 inode
    struct ext2_inode *raw_inode = &fs_info->it_cache.it[index];

    memcpy(raw_inode, ext2_inode , sizeof(struct ext2_inode));
    
    fs_info->it_cache.dirty = true; // 标记为
    return 0;
}

int ext2_sync_fs(struct superblock *sb)
{
    CHECK(sb != NULL && sb->s_private != NULL, "", return -1;);
    ext2_sync_cache(sb); // 同步缓存 
    ext2_sync_super(sb); // 同步superblock到磁盘 
    return 0;
}


struct super_ops ext2_s_ops = {
    .create_private_inode = ext2_create_inode,
    .new_private_inode = ext2_new_inode,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .sync_fs = ext2_sync_fs,
};