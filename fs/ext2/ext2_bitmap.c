#include <fs/types.h>
#include <fs/blkdev.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/err.h>
#include <os/bitops.h>

#include "ext2_types.h"

/* ext2 位图管理：不缓存，每次从磁盘读取到临时缓冲区，修改后直接写回 */

int ext2_bno_group(struct super_block *sb, u32 bno) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);

	if (bno < sbi->s_first_data_block)
		return -1;

	return (bno - sbi->s_first_data_block) / sbi->s_blocks_per_group;
}

int ext2_ino_group(struct super_block *sb, u32 ino) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);

	if (ino == 0 || ino > sbi->s_inodes_count)
		return -1;

	return (ino - 1) / sbi->s_inodes_per_group;
}

static int ext2_read_bitmap(struct super_block *sb, u32 bitmap_block,
			    unsigned long **out_buf) {
	u32 block_size = sb->s_blocksize;
	unsigned long *buf;

	buf = kmalloc(block_size);
	if (!buf)
		return -ENOMEM;

	if (blkdev_read(sb->s_bdev, buf, block_size,
			(u64)bitmap_block * block_size) < 0) {
		kfree(buf);
		return -EIO;
	}

	*out_buf = buf;
	return 0;
}

static int ext2_write_bitmap(struct super_block *sb, u32 bitmap_block,
			     unsigned long *buf) {
	u32 block_size = sb->s_blocksize;

	if (blkdev_write(sb->s_bdev, buf, block_size,
			 (u64)bitmap_block * block_size) < 0)
		return -EIO;

	return 0;
}

/* 在位图缓冲区中扫描第一个为 0 的位 */ 
static int scan_bitmap_0(unsigned long *buf, u32 max_bits) {
	u32 nwords = (max_bits + BITS_PER_LONG - 1) / BITS_PER_LONG;

	for (u32 i = 0; i < nwords; i++) {
		if (buf[i] != ~0UL) {
			int bit = ffz(buf[i]);
			int pos = i * BITS_PER_LONG + bit;
			if ((u32)pos < max_bits)
				return pos;
		}
	}
	return -1;
}

/* 块分配和释放 */
u32 ext2_alloc_bno(struct super_block *sb) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);
	u32 bpg = sbi->s_blocks_per_group;

	for (u32 g = 0; g < sbi->s_groups_count; g++) {
		struct ext2_group_desc *gd = &sbi->gdt[g];
		if (gd->bg_free_blocks_count == 0)
			continue;

		unsigned long *buf = NULL;
		int ret = ext2_read_bitmap(sb, gd->bg_block_bitmap, &buf);
		if (ret < 0)
			continue;

		u32 max_bits = bpg;
		if (g == sbi->s_groups_count - 1) {
			u32 total = sbi->s_blocks_count - sbi->s_first_data_block;
			max_bits = total % bpg;
			if (max_bits == 0)
				max_bits = bpg;
		}

		int bit = scan_bitmap_0(buf, max_bits);
		if (bit < 0) {
			kfree(buf);
			continue;
		}

		__set_bit(bit, (volatile unsigned long *)buf);

		if (ext2_write_bitmap(sb, gd->bg_block_bitmap, buf) < 0) {
			kfree(buf);
			return 0;
		}
		kfree(buf);

		u32 bno = sbi->s_first_data_block + g * bpg + bit;
		gd->bg_free_blocks_count--;
		sbi->raw_sb->s_free_blocks_count--;

		return bno;
	}

	return 0;
}

int ext2_release_bno(struct super_block *sb, u32 bno) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);
	int g = ext2_bno_group(sb, bno);
	if (g < 0)
		return -EINVAL;

	struct ext2_group_desc *gd = &sbi->gdt[g];
	u32 bit = (bno - sbi->s_first_data_block) % sbi->s_blocks_per_group;

	unsigned long *buf = NULL;
	int ret = ext2_read_bitmap(sb, gd->bg_block_bitmap, &buf);
	if (ret < 0)
		return ret;

	__clear_bit(bit, (volatile unsigned long *)buf);

	ret = ext2_write_bitmap(sb, gd->bg_block_bitmap, buf);
	kfree(buf);
	if (ret < 0)
		return ret;

	gd->bg_free_blocks_count++;
	sbi->raw_sb->s_free_blocks_count++;

	return 0;
}

/* inode 分配 释放 */
u32 ext2_alloc_ino(struct super_block *sb) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);
	u32 ipg = sbi->s_inodes_per_group;

	for (u32 g = 0; g < sbi->s_groups_count; g++) {
		struct ext2_group_desc *gd = &sbi->gdt[g];
		if (gd->bg_free_inodes_count == 0)
			continue;

		unsigned long *buf = NULL;
		int ret = ext2_read_bitmap(sb, gd->bg_inode_bitmap, &buf);
		if (ret < 0)
			continue;

		u32 max_bits = ipg;
		if (g == sbi->s_groups_count - 1) {
			u32 remain = sbi->s_inodes_count % ipg;
			if (remain == 0)
				remain = ipg;
			max_bits = remain;
		}

		int bit = scan_bitmap_0(buf, max_bits);
		if (bit < 0) {
			kfree(buf);
			continue;
		}

		__set_bit(bit, (volatile unsigned long *)buf);

		if (ext2_write_bitmap(sb, gd->bg_inode_bitmap, buf) < 0) {
			kfree(buf);
			return 0;
		}
		kfree(buf);

		u32 ino = g * ipg + bit + 1;
		gd->bg_free_inodes_count--;
		sbi->raw_sb->s_free_inodes_count--;

		return ino;
	}

	return 0;
}

int ext2_release_ino(struct super_block *sb, u32 ino) {
	struct ext2_sb_info *sbi = EXT2_SB(sb);
	int g = ext2_ino_group(sb, ino);
	if (g < 0)
		return -EINVAL;

	struct ext2_group_desc *gd = &sbi->gdt[g];
	u32 bit = (ino - 1) % sbi->s_inodes_per_group;

	unsigned long *buf = NULL;
	int ret = ext2_read_bitmap(sb, gd->bg_inode_bitmap, &buf);
	if (ret < 0)
		return ret;

	__clear_bit(bit, (volatile unsigned long *)buf);

	ret = ext2_write_bitmap(sb, gd->bg_inode_bitmap, buf);
	kfree(buf);
	if (ret < 0)
		return ret;

	gd->bg_free_inodes_count++;
	sbi->raw_sb->s_free_inodes_count++;

	return 0;
}
