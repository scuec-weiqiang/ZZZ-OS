/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_inode.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-15 15:04:59
 * @LastEditTime: 2025-09-13 16:13:06
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "ext2_inode.h"
#include "vfs_types.h"
#include "ext2_fs.h"
#include "page_alloc.h"
#include "check.h"
#include "string.h"
#include "spinlock.h"
#include "ext2_cache.h"
#include "time.h"
#include "ext2_block.h"
#include "ext2_page.h"
#include "ext2_namei.h"
#include "ext2_file.h"

uint64_t ext2_ino_group(vfs_superblock_t *vfs_sb,uint64_t ino)
{
    ext2_fs_info_t *fs_info = (ext2_fs_info_t*)vfs_sb->s_private;
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
int64_t ext2_alloc_ino(vfs_superblock_t *vfs_sb)
{
    CHECK(vfs_sb !=NULL && vfs_sb->s_private != NULL, "", return -1;);
    ext2_fs_info_t *fs_info = (ext2_fs_info_t*)vfs_sb->s_private;

    uint64_t group = ext2_select_inode_group(vfs_sb);

    // 加载缓存
    ext2_load_inode_bitmap_cache(vfs_sb,group);
    int64_t free_idx = bitmap_scan_0(fs_info->ibm_cache.ibm);
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

int64_t ext2_release_ino(vfs_superblock_t *vfs_sb, uint64_t ino)
{
    CHECK(vfs_sb != NULL && vfs_sb->s_private != NULL,"",return -1;);
    ext2_fs_info_t *fs_info = (ext2_fs_info_t*)vfs_sb->s_private;
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

void* ext2_create_inode(vfs_inode_t *inode)
{
    CHECK(inode != NULL && inode->i_sb != NULL,"",return NULL;);

    ext2_inode_t *new_private_inode = malloc(sizeof(ext2_inode_t));
    memset(new_private_inode,0,sizeof(ext2_inode_t));
    
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

int64_t ext2_new_inode(vfs_inode_t *inode)
{
    CHECK(inode != NULL && inode->i_sb != NULL && inode->i_sb->s_private != NULL,"",return -1;);
    vfs_superblock_t *vfs_sb = inode->i_sb;

    inode->i_private = ext2_create_inode(inode);
    CHECK(inode->i_private != NULL,"",return -1;);
    int64_t ino = ext2_alloc_ino(vfs_sb);
    CHECK(ino > 0,"",free(inode->i_private);return -1;);
    inode->i_ino = ino;

    // ext2_fs_info_t *fs_info = (ext2_fs_info_t*)vfs_sb->s_private;

    // fs_info->it_cache.dirty = true;
    return ino;
}

static hval_t inode_page_cache_hash(const hlist_node_t* node)
{
    vfs_page_t *page = container_of(node, vfs_page_t, p_lru_cache_node);
    return page->index % 64;
}

static int64_t inode_page_cache_compare(const hlist_node_t* node_a, const hlist_node_t* node_b)
{
    vfs_page_t *a = container_of(node_a, vfs_page_t, p_lru_cache_node);
    vfs_page_t *b = container_of(node_b, vfs_page_t, p_lru_cache_node);
    return a->index - b->index;
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
int64_t ext2_read_inode(vfs_inode_t *inode_ret)
{
    CHECK(inode_ret != NULL, "", return -1;);
    CHECK(inode_ret->i_sb != NULL && inode_ret->i_sb->s_private != NULL, "", return -1;);
    CHECK(inode_ret->i_private != NULL,"",return -1;);

    vfs_superblock_t *vfs_sb = inode_ret->i_sb;
    ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    uint32_t group = ext2_ino_group(vfs_sb,inode_ret->i_ino);
    uint32_t index = inode_ret->i_ino % fs_info->s_inodes_per_group -1;

    // 确保 inode table 已经在内存中
    int64_t ret = ext2_load_inode_table_cache(vfs_sb, group);
    CHECK(ret >= 0, "", return -1;);
  
    // 拿到 ext2 的 inode 指针
    ext2_inode_t *raw_inode = &fs_info->it_cache.it[index];

    // 复制 FS 私有 inode 数据内存
    memcpy(inode_ret->i_private, raw_inode, sizeof(ext2_inode_t));

    // 同步公共字段
    inode_ret->i_size = raw_inode->i_size;  
    inode_ret->i_mode = raw_inode->i_mode;
    inode_ret->i_uid = raw_inode->i_uid;
    inode_ret->i_gid = raw_inode->i_gid;
    inode_ret->i_nlink = raw_inode->i_links_count;
    inode_ret->i_atime.tv_sec = raw_inode->i_atime;
    inode_ret->i_ctime.tv_sec = raw_inode->i_ctime;
    inode_ret->i_mtime.tv_sec = raw_inode->i_mtime;
  
    return inode_ret->i_ino;
}

int64_t ext2_write_inode(vfs_inode_t *inode)
{
    CHECK(inode != NULL, "", return -1;);
    CHECK(inode->i_sb != NULL && inode->i_sb->s_private != NULL, "", return -1;);
    CHECK(inode->i_private != NULL,"",return -1;);

    vfs_superblock_t *vfs_sb = inode->i_sb;
    ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    uint32_t group = ext2_ino_group(vfs_sb,inode->i_ino);
    uint32_t index = inode->i_ino % fs_info->s_inodes_per_group - 1;

    // 确保 inode table 已经在内存中
    int64_t ret = ext2_load_inode_table_cache(vfs_sb, group);
    CHECK(ret >= 0, "", return -1;);

    // 更新缓存中的 inode
    ext2_inode_t *raw_inode = &fs_info->it_cache.it[index];

    memcpy(raw_inode, inode->i_private, sizeof(ext2_inode_t));
    
    fs_info->it_cache.dirty = true; // 标记为
    return 0;
}


vfs_super_ops_t ext2_s_ops = {
    .create_private_inode = ext2_create_inode,
    .new_private_inode = ext2_new_inode,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
};