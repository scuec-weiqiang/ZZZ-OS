#include <fs/types.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include "ext2_types.h"

// 从磁盘读取指定ino的inode信息
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

void dump_raw_inode(struct ext2_inode* inode) {
    printk("size: %xu\n",inode->i_size);
    printk("mode: %x,%s\n",inode->i_mode,EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFREG ? "regular file" :
                                (EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFDIR ? "directory" :
                                (EXT2_GET_TYPE(inode->i_mode) == EXT2_S_IFLNK ? "symlink" : "other")));
    printk("ctime %xu\n", inode->i_ctime);
}

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
    inode->i_mode = raw_inode->i_mode;
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



/* namei.c */
const struct inode_operations ext2_dir_inode_operations = {
    .lookup = ext2_lookup,
};

const struct inode_operations ext2_special_inode_operations = {

};

/* symlink.c */
const struct inode_operations ext2_symlink_inode_operations = {

};
