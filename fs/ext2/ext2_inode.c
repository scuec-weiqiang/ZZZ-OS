#include <fs/types.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>
#include "ext2_types.h"

static u16 mode_to_ext2(u16 vfs_mode) {
	u16 perm = vfs_mode & 07777;
	u16 type = vfs_mode & S_IFMT;
	u16 ext2_type;

	switch (type) {
	case S_IFREG:  ext2_type = EXT2_S_IFREG;  break;
	case S_IFDIR:  ext2_type = EXT2_S_IFDIR;  break;
	case S_IFLNK:  ext2_type = EXT2_S_IFLNK;  break;
	case S_IFCHR:  ext2_type = EXT2_S_IFCHR;  break;
	case S_IFBLK:  ext2_type = EXT2_S_IFBLK;  break;
	case S_IFIFO:  ext2_type = EXT2_S_IFIFO;  break;
	case S_IFSOCK: ext2_type = EXT2_S_IFSOCK; break;
	default:       ext2_type = EXT2_S_IFREG;  break;
	}
	return ext2_type | perm;
}

static u16 mode_from_ext2(u16 ext2_mode) {
	u16 perm = ext2_mode & 07777;
	u16 type = ext2_mode & EXT2_GET_TYPE(ext2_mode);
	u16 vfs_type;

	switch (type) {
	case EXT2_S_IFREG:  vfs_type = S_IFREG;  break;
	case EXT2_S_IFDIR:  vfs_type = S_IFDIR;  break;
	case EXT2_S_IFLNK:  vfs_type = S_IFLNK;  break;
	case EXT2_S_IFCHR:  vfs_type = S_IFCHR;  break;
	case EXT2_S_IFBLK:  vfs_type = S_IFBLK;  break;
	case EXT2_S_IFIFO:  vfs_type = S_IFIFO;  break;
	case EXT2_S_IFSOCK: vfs_type = S_IFSOCK; break;
	default:            vfs_type = S_IFREG;  break;
	}
	return vfs_type | perm;
}

/* Forward declaration */
extern u32 ext2_alloc_ino(struct super_block *sb);
extern int ext2_ino_group(struct super_block *sb, u32 ino);
extern int ext2_release_ino(struct super_block *sb, u32 ino);
extern int ext2_block_mapping(struct inode *inode, u32 index);
extern int ext2_block_set_mapping(struct inode *inode, u32 index);

void dump_raw_inode(struct ext2_inode* inode) {
    printk("size: %xu\n",inode->i_size);
    printk("mode: %x,%s\n",inode->i_mode,EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFREG ? "regular file" :
                                (EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFDIR ? "directory" :
                                (EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFLNK ? "symlink" : "other")));
    printk("ctime %xu\n", inode->i_ctime);
}

/* 从磁盘读取指定ino的inode信息*/
static struct ext2_inode *ext2_get_inode(struct super_block *sb, u32 ino) {
    if (!sb || ino <= 0 || ino > EXT2_SB(sb)->s_inodes_count) {
        return ERR_PTR(-EINVAL);
    }

    struct ext2_sb_info *sbi = EXT2_SB(sb);

    u32 block_size = sb->s_blocksize;
    u32 inode_size = EXT2_SB(sb)->raw_sb->s_inode_size;
    u32 group_no = (ino - 1) / sbi->s_inodes_per_group;

    if (group_no >= sbi->s_groups_count) {
        return ERR_PTR(-EINVAL);
    }

    u32 index_in_group = (ino - 1) % sbi->s_inodes_per_group;
    
    u32 inode_table_block = sbi->gdt[group_no].bg_inode_table;
    u32 inode_offset = index_in_group * inode_size;

    u64 pos = (u64)inode_table_block * block_size + inode_offset;
   
    struct ext2_inode *inode_buf = kmalloc(inode_size);
    if (!inode_buf) {
        return ERR_PTR(-ENOMEM);
    }

    int ret = blkdev_read(sb->s_bdev, (u8*)inode_buf, inode_size, pos);
    if (ret < 0) {
        kfree(inode_buf);
        return ERR_PTR(ret);
    }

    return inode_buf;
}

/* 把inode信息写回磁盘 */
int ext2_write_inode(struct inode *inode) {
	struct ext2_inode_info *ei = EXT2_I(inode);
	struct ext2_sb_info *sbi = EXT2_SB(inode->i_sb);
	u32 block_size = inode->i_sb->s_blocksize;
	u32 inode_size = sbi->raw_sb->s_inode_size;
	u32 ino = inode->i_ino;

	u32 group_no = (ino - 1) / sbi->s_inodes_per_group;
	u32 index_in_group = (ino - 1) % sbi->s_inodes_per_group;
	u32 inode_table_block = sbi->gdt[group_no].bg_inode_table;
	u64 pos = (u64)inode_table_block * block_size + index_in_group * inode_size;

	struct ext2_inode *raw_inode = kmalloc(inode_size);
	if (!raw_inode)
		return -ENOMEM;
	memset(raw_inode, 0, inode_size);

	raw_inode->i_mode = mode_to_ext2(inode->i_mode);
	raw_inode->i_uid = 0;
	raw_inode->i_gid = 0;
	raw_inode->i_size = inode->i_size;
	raw_inode->i_atime = inode->i_atime.tv_sec;
	raw_inode->i_ctime = inode->i_ctime.tv_sec;
	raw_inode->i_mtime = inode->i_mtime.tv_sec;
	raw_inode->i_dtime = ei->i_dtime;
	raw_inode->i_links_count = inode->i_nlink;
	raw_inode->i_blocks = (inode->i_size + 511) / 512;
	raw_inode->i_flags = ei->i_flags;

	for (int n = 0; n < EXT2_N_BLOCKS; n++)
		raw_inode->i_block[n] = ei->i_data[n];

	int ret = blkdev_write(inode->i_sb->s_bdev, (u8*)raw_inode, inode_size, pos);
	kfree(raw_inode);
	return ret < 0 ? ret : 0;
}

/* 新分配一个磁盘里没有的ext2_inode，并返回vfs的inode */
struct inode *ext2_new_inode(struct inode *dir, u16 mode) {
	struct super_block *sb = dir->i_sb;
	struct ext2_inode_info *ei;
	struct inode *inode;
	u32 ino;

	ino = ext2_alloc_ino(sb);
	if (ino == 0)
		return ERR_PTR(-ENOSPC);

	inode = iget(sb, ino);
	if (IS_ERR(inode))
		return inode;

	if (inode->i_state != I_NEW) {
		/* Shouldn't happen for a freshly allocated inode */
		return ERR_PTR(-EIO);
	}

	ei = EXT2_I(inode);
	inode->i_mode = mode;
	inode->i_nlink = S_ISDIR(mode) ? 2 : 1;
	inode->i_size = 0;
	inode->i_atime = inode->i_ctime = inode->i_mtime =
		(timespec_t){ .tv_sec = 0, .tv_nsec = 0 };
	inode->i_state = I_DIRTY;

	ei->i_flags = 0;
	ei->i_dtime = 0;
	ei->i_state = EXT2_STATE_NEW;
	ei->i_block_group = ext2_ino_group(sb, ino);
	ei->i_dir_start_lookup = 0;
	memset(ei->i_data, 0, sizeof(ei->i_data));

	if (S_ISREG(mode)) {
		inode->i_op = &ext2_file_inode_operations;
		inode->i_mapping->a_ops = &ext2_aops;
		inode->i_fop = &ext2_file_operations;
	} else if (S_ISDIR(mode)) {
		inode->i_op = &ext2_dir_inode_operations;
		inode->i_fop = &ext2_dir_operations;
		inode->i_mapping->a_ops = &ext2_aops;
	} else {
		inode->i_op = &ext2_special_inode_operations;
	}

	int ret = ext2_write_inode(inode);
	if (ret < 0) {
		ext2_release_ino(sb, ino);
		iput(inode);
		return ERR_PTR(ret);
	}

	return inode;
}

/* 获得一个磁盘里已建立的inode*/
struct inode* ext2_iget(struct super_block *sb, u32 ino) {
	struct ext2_inode_info *ei;
	struct ext2_inode *raw_inode;
	struct inode *inode;

    inode = iget(sb, ino);
    if (IS_ERR(inode)) {
        return inode;
    }

    if (inode->i_state != I_NEW) {
        
        return inode;
    }

    ei = EXT2_I(inode);


    raw_inode = ext2_get_inode(sb, ino);
    if (IS_ERR(raw_inode)) {
        iput(inode);
        return ERR_CAST(raw_inode);
    }

    // dump_raw_inode(raw_inode);

    inode->i_nlink = raw_inode->i_links_count;
    inode->i_mode = mode_from_ext2(raw_inode->i_mode);
    inode->i_size = raw_inode->i_size;
    inode->i_atime.tv_sec = raw_inode->i_atime;
    inode->i_ctime.tv_sec = raw_inode->i_ctime;
    inode->i_mtime.tv_sec = raw_inode->i_mtime;
    ei->i_dtime = raw_inode->i_dtime;


    if (inode->i_nlink == 0 && (inode->i_mode == 0 || ei->i_dtime)) {
        kfree(raw_inode);
        iput(inode);
        return ERR_PTR(-ENOENT);
    }
    ei->i_flags = raw_inode->i_flags;
    ei->i_dtime = 0;
	ei->i_state = 0;
	ei->i_block_group = (ino - 1) / EXT2_SB(sb)->s_inodes_per_group;
	ei->i_dir_start_lookup = 0;

    for (int n = 0; n < EXT2_N_BLOCKS; n++) {
		ei->i_data[n] = raw_inode->i_block[n];
    }

    if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ext2_file_inode_operations;
        inode->i_mapping->a_ops = &ext2_aops;
        inode->i_fop = &ext2_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ext2_dir_inode_operations;
		inode->i_fop = &ext2_dir_operations;
		inode->i_mapping->a_ops = &ext2_aops;
	} else if (S_ISLNK(inode->i_mode)) {
        inode->i_op = &ext2_symlink_inode_operations;
        inode->i_mapping->a_ops = &ext2_aops;
	} else {
        inode->i_op = &ext2_special_inode_operations;
    }
    kfree(raw_inode);
    return inode;
}

extern u32 ext2_find(struct inode *dir, struct qstr *child);

struct dentry * ext2_lookup(struct inode *dir,struct dentry *child_dentry, unsigned int flags) {
    u32 ino = ext2_find(dir, &child_dentry->d_name);
    if (ino) {
        struct inode *child_inode = ext2_iget(dir->i_sb, ino);
        if (IS_ERR(child_inode)) {
            return ERR_CAST(child_inode);
        }
        d_add(child_dentry, child_inode);
        return child_dentry;
    } else {
        return ERR_PTR(-ENOENT);
    }
}

/* file.c */
const struct inode_operations ext2_file_inode_operations = {
};

const struct inode_operations ext2_special_inode_operations = {
};

/* symlink.c */
const struct inode_operations ext2_symlink_inode_operations = {

};
