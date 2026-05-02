#include <fs/types.h>
#include <fs/blkdev.h>

#include <os/kmalloc.h>
#include <os/err.h>

#include "ext2_types.h"

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
int ext2_block_mapping(struct inode *inode, u32 index) {
    struct ext2_inode_info *ei = EXT2_I(inode);
    struct blkdev *bdev = inode->i_sb->s_bdev;

    u32 block_size = inode->i_sb->s_blocksize;
    u32 per_block = block_size / sizeof(u32);

    u32 first_index = 0;
    u32 second_index = 0;
    u32 third_index = 0;
    int next_level_index = 0;
    int ret = -ENOMEM;

    // 直接索引
    if (index < 12)
    {
        return ei->i_data[index];
    }

    
    void *buf = kmalloc(block_size);
    if (!buf) {
        return -ENOMEM;
    }

    // 一级间接索引
    index -= 12;
    if (index < per_block)
    {
        blkdev_read(bdev, buf, block_size, (u64)ei->i_data[12] * block_size);
        ret = ((u32*)buf)[index];
        goto got;
    }

    // 二级间接索引
    index -= per_block;
    if (index < per_block * per_block)
    {
        // 计算索引的过程类似于把数字245提取出百位数字2,十位数字4,个位数字5，道理是一样的
        first_index = index / per_block;
        blkdev_read(bdev, buf, block_size, (u64)ei->i_data[13] * block_size);
        next_level_index = ((u32*)buf)[first_index];

        second_index = index % per_block;
        blkdev_read(bdev, buf, block_size, (u64)next_level_index * block_size);

        ret = ((u32*)buf)[second_index];
    
        return ret;
    }

    // 三级间接索引
    index -= per_block * per_block;
    if (index < per_block * per_block * per_block) {
        // 计算索引的过程类似于把数字245提取出百位数字2,十位数字4,个位数字5，道理是一样的
        first_index = index / (per_block * per_block);
        blkdev_read(bdev, buf, block_size, (u64)ei->i_data[14] * block_size);
        next_level_index = ((u32*)buf)[first_index];

        second_index = (index % (per_block * per_block) ) / per_block;
        blkdev_read(bdev, buf, block_size, (u64)next_level_index * block_size);

        third_index = index % per_block;
        blkdev_read(bdev, buf, block_size, (u64)next_level_index * block_size);

        ret = ((u32*)buf)[third_index];
        goto got;
    }
    got:
        kfree(buf);
        return ret;

    return -ENOENT;
}
  