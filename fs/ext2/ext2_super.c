#include <fs/types.h>
#include <fs/super.h>
#include <fs/dcache.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/fs_context.h>
#include <os/kmalloc.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/init.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/utils.h>
#include "ext2_types.h"

static struct inode* ext2_alloc_inode(struct super_block *sb) {
    struct ext2_inode_info *ei;
    ei = kmalloc(sizeof(struct ext2_inode_info));
	if (!ei)
		return ERR_PTR(-ENOMEM);

    memset(ei, 0, sizeof(struct ext2_inode_info));
    // printk("ext2_alloc_inode: ei=%xu vfs_inode=%xu offset=%xu size=%xu\n",
    //        ei, &ei->vfs_inode,
    //        (u32)((uintptr_t)&ei->vfs_inode - (uintptr_t)ei),
    //        sizeof(struct ext2_inode_info));
    return &ei->vfs_inode;
}

static void ext2_destroy_inode(struct inode *inode) {
    if (!inode) {
        return;
    }
    kfree(EXT2_I(inode));
}

struct super_operations ext2_super_ops = {
    .alloc_inode = ext2_alloc_inode,
    .destroy_inode = ext2_destroy_inode,
};

static int ext2_init_fs_context(struct fs_context *fc) {
    return 0;
}

void dump_ext2_sb(struct ext2_super_block *sb) {
    printk("s_inodes_count: %xu\n", sb->s_inodes_count);
    printk("s_blocks_count: %xu\n", sb->s_blocks_count);
    printk("s_free_blocks_count: %xu\n", sb->s_free_blocks_count);
    printk("s_free_inodes_count: %xu\n", sb->s_free_inodes_count);
    printk("s_first_data_block: %xu\n", sb->s_first_data_block);
    printk("s_log_block_size: %xu\n", sb->s_log_block_size);
    printk("s_log_frag_size: %xu\n", sb->s_log_frag_size);
    printk("s_blocks_per_group: %xu\n", sb->s_blocks_per_group);
    printk("s_frags_per_group: %xu\n", sb->s_frags_per_group);
    printk("s_inodes_per_group: %xu\n", sb->s_inodes_per_group);
    printk("s_mtime: %xxu\n", sb->s_mtime);
    printk("s_wtime: %xu\n", sb->s_wtime);
    printk("s_mnt_count: %xu\n", sb->s_mnt_count);
    printk("s_max_mnt_count: %d\n", sb->s_max_mnt_count);
    printk("s_magic: %xu\n", sb->s_magic);
    printk("s_block_group_nr: %xu\n", sb->s_block_group_nr);
}

static int ext2_get_tree(struct fs_context *fc) {
    struct super_block *sb = NULL;
    struct ext2_sb_info *sbi = NULL;
    struct ext2_super_block *raw_sb;
    struct ext2_group_desc  *raw_gdt;

    struct inode *root_inode = NULL;
    struct dentry *root_dentry = NULL;

    struct blkdev *bdev = NULL;
    int ret = 0;
    u32 block_size = 0;
    u32 blocks_after_first = 0;

    bdev = blkdev_get_by_path(fc->source);
    if (IS_ERR(bdev)) {
        ret = -ENODEV;
        goto failed;
    }
    
    // 先读取超级块信息
    raw_sb = (struct ext2_super_block*)kmalloc(EXT2_SUPERBLOCK_SIZE);
    if (!raw_sb) {
        ret = -ENOMEM;
        goto fail_raw_sb;
    }
    if (blkdev_read(bdev, raw_sb, EXT2_SUPERBLOCK_SIZE, EXT2_SUPERBLOCK_OFFSET) < 0) {
        ret = -EIO;
        goto fail_read_sb;
    }

    if (raw_sb->s_magic != EXT2_SUPER_MAGIC) {
        ret = -EINVAL;
        goto bad_magic;
    }

    sbi = kmalloc(sizeof(struct ext2_sb_info));
    if (!sbi) {
        
        ret = -ENOMEM;
        goto fail_sbi;
    }

    sbi->raw_sb = raw_sb;

    sbi->s_blocks_per_group = raw_sb->s_blocks_per_group;
    sbi->s_inodes_per_group = raw_sb->s_inodes_per_group;

    if (raw_sb->s_blocks_count <= raw_sb->s_first_data_block) {
        ret = -EINVAL;
        goto fail_sbi;
    }
    if (raw_sb->s_blocks_per_group == 0 || raw_sb->s_inodes_per_group == 0) {
        ret = -EINVAL;
        goto fail_sbi;
    }
    if (raw_sb->s_inode_size == 0) {
        ret = -EINVAL;
        goto fail_sbi;
    }

    block_size = 1024U << raw_sb->s_log_block_size;
    // 不支持超过4k的块大小
    if (block_size > PAGE_SIZE) {
        ret = -EINVAL;
        goto fail_sbi;
    }
    blocks_after_first = raw_sb->s_blocks_count - raw_sb->s_first_data_block;

    // 一个group中的inode table占的block数
    sbi->s_itb_per_group = (sbi->s_inodes_per_group * raw_sb->s_inode_size + block_size - 1) / block_size;

    sbi->s_ibmb_per_group = (((sbi->s_inodes_per_group + 7) / 8) + block_size - 1 )/ block_size;
    sbi->s_bbmb_per_group = (((sbi->s_blocks_per_group + 7) / 8) + block_size - 1)/ block_size;

    sbi->s_inodes_per_block = block_size / raw_sb->s_inode_size;
    sbi->s_desc_per_block = block_size / sizeof(struct ext2_group_desc);

    sbi->s_groups_count = (blocks_after_first + raw_sb->s_blocks_per_group - 1) / raw_sb->s_blocks_per_group;
    sbi->s_gdb_count = (sbi->s_groups_count * sizeof(struct ext2_group_desc) + block_size - 1) / block_size;

    sbi->s_inodes_count = raw_sb->s_inodes_count;
    sbi->s_blocks_count = raw_sb->s_blocks_count;
    sbi->s_free_blocks_count = raw_sb->s_free_blocks_count;
    sbi->s_free_inodes_count = raw_sb->s_free_inodes_count;
    sbi->s_first_data_block = raw_sb->s_first_data_block;

    raw_gdt = (struct ext2_group_desc*)kmalloc(sbi->s_gdb_count * block_size);
    if (!raw_gdt) {
        
        ret = -ENOMEM;
        goto fail_gdt;
    }
    u32 gdt_offset = (sbi->s_first_data_block + 1) * block_size; //块描述符表起始位置

    if (blkdev_read(bdev, raw_gdt, sbi->s_gdb_count * block_size, gdt_offset) < 0) {
        ret = -EIO;
        goto fail_gdt;
    }
    sbi->gdt = raw_gdt;


    sb = alloc_super(fc->fs_type);
    if (!sb) {
        
        ret = -ENOMEM;
        goto fail_sb;
    }
    sb->s_fs_info = sbi;
    sb->s_op = &ext2_super_ops;
    sb->s_bdev = bdev;
    sb->s_blocksize = block_size;

    // Create the root inode and dentry
    root_inode = ext2_iget(sb, EXT2_ROOT_INO);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto fail_root_inode;
    }

    printk("root inode: mode=%xu,size=%xu,state=%xu\n",
           root_inode->i_mode, root_inode->i_size, root_inode->i_state);

    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        goto fail_root_dentry;
        ret =  -ENOMEM;
    }

    sb->s_root = root_dentry;

    fc->root = root_dentry;
    
    return 0;

fail_root_dentry:
    iput(root_inode);
fail_root_inode:
fail_gdt:
fail_sb:
    kfree(sbi);
fail_sbi:
bad_magic:
fail_read_sb:
    kfree(raw_sb);
fail_raw_sb:
failed:
    return ret;
}

static void ext2_kill_sb(struct super_block *sb) {

}
struct file_system_type ext2_fs_type = {
    .name = "ext2",
    .init_fs_context = ext2_init_fs_context,
    .get_tree = ext2_get_tree,
    .kill_sb = ext2_kill_sb,
};

static int ext2_register_filesystem_init(void)
{
    return register_filesystem(&ext2_fs_type);
}

fs_initcall(ext2_register_filesystem_init);
