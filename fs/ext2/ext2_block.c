/**
 * @FilePath: /ZZZ-OS/fs/ext2/ext2_block.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 18:21:24
 * @LastEditTime: 2025-10-29 22:24:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "fs/vfs_types.h"
#include "fs/ext2/ext2_types.h"
#include "os/malloc.h"
#include "os/check.h"
#include "asm/spinlock.h"
#include "fs/ext2/ext2_cache.h"

int ext2_bno_group(struct superblock *vfs_sb, uint64_t bno)
{
    CHECK(vfs_sb != NULL, "", return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info *)vfs_sb->s_private;
    return (bno) / fs_info->s_blocks_per_group;
}

struct spinlock ext2_balloc_lock = SPINLOCK_INIT;
struct spinlock ext2_bfree_lock = SPINLOCK_INIT;

#define BALLOC_LOCK spin_lock(&ext2_balloc_lock)
#define BALLOC_UNLOCK spin_unlock(&ext2_balloc_lock)
#define BFREE_LOCK spin_lock(&ext2_bfree_lock)
#define BFREE_UNLOCK spin_unlock(&ext2_bfree_lock)

/**
 * @brief 在ext2文件系统中为给定组分配一个块号
 *
 * 从指定的文件系统中为给定组分配一个空闲块，并返回该块的块号。
 *
 * @param vfs_sb 文件系统超级块指针
 * @param group 要分配块的组号
 *
 * @return 分配的块号，如果分配失败则返回-1
 */
int ext2_alloc_bno(struct superblock *vfs_sb)
{
    // BALLOC_LOCK;

    CHECK(vfs_sb != NULL, "", return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info *)vfs_sb->s_private;
    // 加载缓存
    int group = ext2_select_block_group(vfs_sb);
    ext2_load_block_bitmap_cache(vfs_sb, group);
    int free_idx = bitmap_scan_0(fs_info->bbm_cache.bbm);
    CHECK(free_idx >= 0, "", return -1;);

    bitmap_set_bit(fs_info->bbm_cache.bbm, free_idx);
    fs_info->bbm_cache.dirty = true;
    fs_info->s_free_blocks_count--;
    fs_info->super->s_free_blocks_count--;
    fs_info->group_desc[group].bg_free_blocks_count--;
    free_idx += group * fs_info->s_blocks_per_group;

    // BALLOC_UNLOCK;

    return free_idx;
}

/**
 * @brief 释放一个块号
 *
 * 从文件系统中释放一个块号，并更新相应的文件系统信息。
 *
 * @param vfs_sb VFS超级块指针
 * @param bno 要释放的块号
 *
 * @return 成功时返回释放的块号，失败时返回-1
 */
int ext2_release_bno(struct superblock *vfs_sb, uint64_t bno)
{
    BFREE_LOCK;

    CHECK(vfs_sb != NULL, "", return -1;);

    struct ext2_fs_info *fs_info = (struct ext2_fs_info *)vfs_sb->s_private;
    struct bitmap *bm = NULL;
    uint32_t group = ext2_bno_group(vfs_sb, bno);
    ext2_load_block_bitmap_cache(vfs_sb, group);
    bm = fs_info->bbm_cache.bbm;
    int ret = bitmap_clear_bit(bm, bno);
    CHECK(ret >= 0, "", return -1;);

    fs_info->bbm_cache.dirty = true;
    fs_info->s_free_blocks_count++;
    fs_info->super->s_free_blocks_count++;
    fs_info->group_desc[group].bg_free_blocks_count++;

    BFREE_UNLOCK;

    return bno;
}

static int find_sub(struct block_adapter *adap, uint32_t block_index, uint32_t index)
{
    CHECK(adap != NULL, "", return -1;);
    uint32_t block_size = block_adapter_get_block_size(adap);
    uint32_t *buf = malloc(block_size);
    CHECK(buf != NULL, "", return -1;);
    int ret = block_adapter_read(adap, buf, block_index, 1);
    CHECK(ret >= 0, "", return -1;);
    ret = (int)buf[index];
    free(buf);
    return ret;
}

/**
 * @brief 将 VFS 索引节点中的索引映射为 ext2 文件系统中的块地址
 *
 * 该函数将 VFS 索引节点中的索引映射为 ext2 文件系统中的块地址。
 *
 * @param inode VFS 索引节点指针
 * @param index 索引值
 *
 * @return 映射得到的块地址，如果映射失败则返回 -1
 */
int ext2_block_mapping(struct inode *inode, uint64_t index)
{
    CHECK(inode != NULL, "", return -1;);
    CHECK(inode->i_sb->s_private != NULL, "", return -1;);
    uint32_t block_size = inode->i_sb->s_block_size;
    uint32_t per_block = block_size / sizeof(uint32_t);
    struct ext2_inode *ext2_inode = (struct ext2_inode *)inode->i_private;
    struct block_adapter *adap = (struct block_adapter *)inode->i_sb->adap;
    uint64_t first_index = 0;
    uint64_t second_index = 0;
    uint64_t third_index = 0;
    int sub_block_index = 0;
    int ret = -1;
    // 直接索引
    if (index < 12)
    {
        return ext2_inode->i_block[index];
    }

    // 一级间接索引
    index -= 12;
    if (index < per_block)
    {
        return find_sub(adap, ext2_inode->i_block[12], index);
    }

    // 二级间接索引
    index -= per_block;
    if (index < per_block * per_block)
    {
        // 计算索引的过程类似于把数字245提取出百位数字2,十位数字4,个位数字5，道理是一样的
        first_index = index / per_block;
        sub_block_index = find_sub(adap, ext2_inode->i_block[13], first_index);
        second_index = index % per_block;
        ret = find_sub(adap, sub_block_index, second_index);
        return ret;
    }

    // 三级间接索引
    index -= per_block * per_block;
    if (index < per_block * per_block * per_block)
    {
        first_index = index / (per_block * per_block);
        sub_block_index = find_sub(adap, ext2_inode->i_block[14], first_index);
        second_index = index % (per_block * per_block) / per_block;
        sub_block_index = find_sub(adap, sub_block_index, second_index);
        third_index = index % per_block;
        ret = find_sub(adap, sub_block_index, third_index);
        return ret;
    }
    return -1;
}
