#include <fs/types.h>
#include <fs/blkdev.h>

#include <os/kmalloc.h>
#include <os/err.h>
#include <os/string.h>
#include <os/printk.h>
#include "ext2_types.h"

extern u32 ext2_alloc_bno(struct super_block *sb);

int ext2_block_mapping(struct inode *inode, u32 index) {
    struct ext2_inode_info *ei = EXT2_I(inode);
    u32 block_size = inode->i_sb->s_blocksize;
    u32 per_block = block_size / sizeof(u32);

    u32 first_index = 0;
    u32 second_index = 0;
    u32 third_index = 0;
    int next_level_index = 0;
    int ret = -ENOENT;

    if (index < 12) {
        return ei->i_data[index];
    }

    void *buf = kmalloc(block_size);
    if (!buf)
        return -ENOMEM;

    index -= 12;

    if (index < per_block) {
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)ei->i_data[12] * block_size);
        ret = ((u32*)buf)[index];
        goto got;
    }

    index -= per_block;

    if (index < per_block * per_block) {
        first_index = index / per_block;
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)ei->i_data[13] * block_size);
        next_level_index = ((u32*)buf)[first_index];

        second_index = index % per_block;
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)next_level_index * block_size);

        ret = ((u32*)buf)[second_index];
        goto got;
    }

    index -= per_block * per_block;


    if (index < per_block * per_block * per_block) {
        first_index = index / (per_block * per_block);
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)ei->i_data[14] * block_size);
        next_level_index = ((u32*)buf)[first_index];

        second_index = (index % (per_block * per_block)) / per_block;
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)next_level_index * block_size);
        int third_level_index = ((u32*)buf)[second_index];

        third_index = index % per_block;
        blkdev_read(inode->i_sb->s_bdev, buf, block_size,
                    (u64)third_level_index * block_size);

        ret = ((u32*)buf)[third_index];
        goto got;
    }

got:
    kfree(buf);
    return ret;
}

static int read_or_alloc_block(struct super_block *sb, u32 *bno_p, void *buf, u32 block_size) {
    if (*bno_p == 0) {
        u32 new_bno = ext2_alloc_bno(sb);
        if (new_bno == 0) {
            return -ENOSPC;
        }
            
        *bno_p = new_bno;
        memset(buf, 0, block_size);
        blkdev_write(sb->s_bdev, buf, block_size, (u64)new_bno * block_size);
        return 0;
    }
    return blkdev_read(sb->s_bdev, buf, block_size, (u64)*bno_p * block_size);
}

int ext2_block_set_mapping(struct inode *inode, u32 index) {
    
    struct ext2_inode_info *ei = EXT2_I(inode);
    struct super_block *sb = inode->i_sb;
    u32 block_size = sb->s_blocksize;
    u32 per_block = block_size / sizeof(u32);

    void *buf = kmalloc(block_size);
    if (!buf)
        return -ENOMEM;

    if (index < 12) {
        
        if (ei->i_data[index] == 0) {
            u32 bno = ext2_alloc_bno(sb);
            if (bno == 0) {
                kfree(buf);
                
                return -ENOSPC;
            }
            ei->i_data[index] = bno;
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)bno * block_size);
        }
        kfree(buf);
        
        return ei->i_data[index];
    }

    index -= 12;

 
    if (index < per_block) {
        int ret = read_or_alloc_block(sb, &ei->i_data[12], buf, block_size);
        if (ret < 0) { kfree(buf); return ret; }

        u32 bno = ((u32*)buf)[index];
        if (bno == 0) {
            bno = ext2_alloc_bno(sb);
            if (bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[index] = bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)ei->i_data[12] * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)bno * block_size);
        }
        kfree(buf);
        return bno;
    }

    index -= per_block;


    if (index < per_block * per_block) {
        u32 first_index = index / per_block;
        u32 second_index = index % per_block;

        int ret = read_or_alloc_block(sb, &ei->i_data[13], buf, block_size);
        if (ret < 0) { kfree(buf); return ret; }

        u32 first_bno = ((u32*)buf)[first_index];
        if (first_bno == 0) {
            first_bno = ext2_alloc_bno(sb);
            if (first_bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[first_index] = first_bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)ei->i_data[13] * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
        }

        blkdev_read(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
        u32 second_bno = ((u32*)buf)[second_index];
        if (second_bno == 0) {
            second_bno = ext2_alloc_bno(sb);
            if (second_bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[second_index] = second_bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)second_bno * block_size);
        }
        kfree(buf);
        return second_bno;
    }

    index -= per_block * per_block;

 
    if (index < per_block * per_block * per_block) {
        u32 first_index = index / (per_block * per_block);
        u32 second_index = (index % (per_block * per_block)) / per_block;
        u32 third_index = index % per_block;

        int ret = read_or_alloc_block(sb, &ei->i_data[14], buf, block_size);
        if (ret < 0) { kfree(buf); return ret; }

        u32 first_bno = ((u32*)buf)[first_index];
        if (first_bno == 0) {
            first_bno = ext2_alloc_bno(sb);
            if (first_bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[first_index] = first_bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)ei->i_data[14] * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
        }

        blkdev_read(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
        u32 second_bno = ((u32*)buf)[second_index];
        if (second_bno == 0) {
            second_bno = ext2_alloc_bno(sb);
            if (second_bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[second_index] = second_bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)first_bno * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)second_bno * block_size);
        }

        blkdev_read(sb->s_bdev, buf, block_size, (u64)second_bno * block_size);
        u32 third_bno = ((u32*)buf)[third_index];
        if (third_bno == 0) {
            third_bno = ext2_alloc_bno(sb);
            if (third_bno == 0) { kfree(buf); return -ENOSPC; }
            ((u32*)buf)[third_index] = third_bno;
            blkdev_write(sb->s_bdev, buf, block_size, (u64)second_bno * block_size);
            memset(buf, 0, block_size);
            blkdev_write(sb->s_bdev, buf, block_size, (u64)third_bno * block_size);
        }
        kfree(buf);
        return third_bno;
    }

    kfree(buf);
    return -EFBIG;
}
