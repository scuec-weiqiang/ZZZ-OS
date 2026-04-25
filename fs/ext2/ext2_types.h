#ifndef EXT2_H
#define EXT2_H

#include <os/types.h>
#include <os/bitmap.h>
#include <fs/types.h>

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPERBLOCK_SIZE   1024
#define EXT2_SUPER_MAGIC 0xEF53
struct __attribute__((packed)) ext2_super_block {

    __le32 s_inodes_count;      // inode数
    __le32 s_blocks_count;      // 块数
    __le32 s_r_blocks_count;    // 保留块数
    __le32 s_free_blocks_count; // 空闲块数
    __le32 s_free_inodes_count; // 空闲inode数
    __le32 s_first_data_block;  // 第一个数据块索引(通常是1 for 1k block, 0 for >1k)
    __le32 s_log_block_size;    // 块大小的对数值,实际块大小=(1024 << s_log_block_size)
    __le32 s_log_frag_size;     // 碎片大小的对数值，同上
    __le32 s_blocks_per_group;  // 每个组的块数
    __le32 s_frags_per_group;   // 每个块组多少碎片（已废弃）
    __le32 s_inodes_per_group;  // 每个组的inode数
    __le32 s_mtime;             // 最后挂载时间
    __le32 s_wtime;             // 最后写入时间
    __le16 s_mnt_count;         // 挂载次数
    __le16 s_max_mnt_count;     // 最大挂载次数
    __le16 s_magic;             // 文件系统魔数，必须是0xEF53
    __le16 s_state;             // 文件系统状态
    __le16 s_errors;            // 错误处理策略
    __le16 s_minor_rev_level;   // 次版本号
    __le32 s_lastcheck;         // 最后检查时间
    __le32 s_checkinterval;     // 检查间隔
    __le32 s_creator_os;        // 创建 OS
    __le32 s_rev_level;         // 修订版本
    __le16 s_def_resuid;        // 默认保留 uid
    __le16 s_def_resgid;        // 默认保留 gid
    // ... 其他字段
    __le32 s_first_ino;              /* First non-reserved inode */
    __le16 s_inode_size;             /* size of inode structure */
    __le16 s_block_group_nr;         /* block group # of this superblock */
    __le32 s_feature_compat;         /* compatible feature set */
    __le32 s_feature_incompat;       /* incompatible feature set */
    __le32 s_feature_ro_compat;      /* readonly-compatible feature set */
    __u8 s_uuid[16];                /* 128-bit uuid for volume */
    char s_volume_name[16];            /* volume name */
    char s_last_mounted[64];           /* directory where last mounted */
    __le32 s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    __u8 s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    __u8 s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    __le16 s_padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    __u8 s_journal_uuid[16]; /* uuid of journal superblock */
    __le32 s_journal_inum;    /* inode number of journal file */
    __le32 s_journal_dev;     /* device number of journal file */
    __le32 s_last_orphan;     /* start of list of inodes to delete */
    __le32 s_hash_seed[4];    /* HTREE hash seed */
    __u8 s_def_hash_version; /* Default hash version to use */
    __u8 s_reserved_char_pad;
    __le16 s_reserved_word_pad;
    __le32 s_default_mount_opts;
    __le32 s_first_meta_bg; /* First metablock block group */
    __le32 s_reserved[190]; /* Padding to the end of the block */
};

struct __attribute__((packed)) ext2_group_desc {
    __le32 bg_block_bitmap;      // 块位图的块号
    __le32 bg_inode_bitmap;      // inode位图的块号
    __le32 bg_inode_table;       // inode表的起始块号
    __le16 bg_free_blocks_count; // 组内空闲块数
    __le16 bg_free_inodes_count; // 组内空闲inode数
    __le16 bg_used_dirs_count;   // 组内目录数
    __le16 bg_pad;
    __le32 bg_reserved[3];
};

#define EXT2_GET_TYPE(x) ((x) & 0xF000)

/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

#define EXT2_ROOT_INO 2 //根目录的编号
// 注意索引号从0开始，而ext2中的entry存储的inode编号是从1开始的
// inode编号	 用途      inode table中的索引号
//     1	  损坏块列表           0
//     2       根目录             1
//     3	  ACL 索引            2
//     4	  ACL 数据            3
// 例如如果从ext2的entry中获取inode编号为0xb（11），那么对应的inode表下标为10

#define EXT2_S_IFSOCK 0xC000 // socket
#define EXT2_S_IFLNK 0xA000  // symbolic link
#define EXT2_S_IFREG 0x8000  // regular file
#define EXT2_S_IFBLK 0x6000  // block device
#define EXT2_S_IFDIR 0x4000  // directory
#define EXT2_S_IFCHR 0x2000  // character device
#define EXT2_S_IFIFO 0x1000  // FIFO

// 权限
#define EXT2_S_IDEFAULT 0x01ed
#define EXT2_S_IRUSR 0x0100 // owner read
#define EXT2_S_IWUSR 0x0080 // owner write
#define EXT2_S_IXUSR 0x0040 // owner execute
#define EXT2_S_IRGRP 0x0020 // group read
#define EXT2_S_IWGRP 0x0010 // group write
#define EXT2_S_IXGRP 0x0008 // group execute
#define EXT2_S_IROTH 0x0004 // others read
#define EXT2_S_IWOTH 0x0002 // others write
#define EXT2_S_IXOTH 0x0001 // others execute
struct __attribute__((packed)) ext2_inode  {
    __le16 i_mode;        // 文件类型和权限
    __le16 i_uid;         // 用户ID
    __le32 i_size;        // 文件大小（字节）
    __le32 i_atime;       // 最后访问时间
    __le32 i_ctime;       // 创建时间
    __le32 i_mtime;       // 最后修改时间
    __le32 i_dtime;       // 删除时间
    __le16 i_gid;         // 组ID
    __le16 i_links_count; // 硬链接计数
    __le32 i_blocks;      // 文件占用块数（以512字节为单位）
    __le32 i_flags;       // 文件标志
    union
    {
        struct
        {
            __le32 l_i_reserved1;
        } linux1;
        struct
        {
            __le32 h_i_translator;
        } hurd1;
        struct
        {
            __le32 m_i_reserved1;
        } masix1;
    } osd1; /* OS dependent 1 */ // 保留字段
    __le32 i_block[15];   // 文件占据的块号（12直接，1个间接，1个双重间接，1个三重间接）
    __le32 i_generation;  // 文件版本
    __le32 i_file_acl;    // ACL（稀有）
    __le32 i_dir_acl;     // 目录 ACL
    __le32 i_faddr;       // 碎片地址
    union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__le16	i_pad1;
			__le16	l_i_uid_high;	/* these 2 fields    */
			__le16	l_i_gid_high;	/* were reserved2[0] */
			__le32	l_i_reserved2;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__le16	h_i_mode_high;
			__le16	h_i_uid_high;
			__le16	h_i_gid_high;
			__le32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__le16	m_pad1;
			__le32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

enum {
	EXT2_FT_UNKNOWN		= 0,
	EXT2_FT_REG_FILE	= 1,
	EXT2_FT_DIR		= 2,
	EXT2_FT_CHRDEV		= 3,
	EXT2_FT_BLKDEV		= 4,
	EXT2_FT_FIFO		= 5,
	EXT2_FT_SOCK		= 6,
	EXT2_FT_SYMLINK		= 7,
	EXT2_FT_MAX
};


/*
 * EXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
 #define EXT2_DIR_PAD		 	4
 #define EXT2_DIR_ROUND 			(EXT2_DIR_PAD - 1)
 #define EXT2_DIR_REC_LEN(name_len)	(((name_len) + 8 + EXT2_DIR_ROUND) & \
                      ~EXT2_DIR_ROUND)
 #define EXT2_MAX_REC_LEN		((1<<16)-1)

struct __attribute__((packed)) ext2_dir_entry_2 {
    __le32 inode;    // inode号 (0表示该条目未使用)
    __le16 rec_len;  // 该目录项的总长度
    __u8 name_len;  // 文件名长度
    __u8 file_type; // 文件类型
    char name[];       // 文件名
};

struct ext2_sb_info {
    struct ext2_super_block *raw_sb;
    struct ext2_group_desc *gdt;

    __le32 s_blocks_per_group;
    __le32 s_inodes_per_group;
    __le32 s_itb_per_group;     // 一个group中的inode table占的block数
    __le32 s_ibmb_per_group; // 每个group的inode位图占的block数
    __le32 s_bbmb_per_group; // 每个group的block位图占的block数

    __le32 s_inodes_per_block;
    __le32 s_desc_per_block;    // 一个block中能放多少个group descripter

    __le32 s_gdb_count;         // 所有group descripter占的block数
    __le32 s_groups_count;      // 有多少块组
    __le32 s_inodes_count;      // inode数
    __le32 s_blocks_count;      // 块数
    __le32 s_free_blocks_count; // 空闲块数
    __le32 s_free_inodes_count; // 空闲inode数

    __le32 s_first_data_block;  // 第一个数据块索引(通常是1 for 1k block, 0 for >1k)
};

/*
 * second extended file system inode data in memory
 */
struct ext2_inode_info {
	__le32	i_data[15];
	__u32	i_flags;
	__u32	i_faddr;

	__u16	i_state;
	__u32	i_file_acl;
	__u32	i_dir_acl;
	__u32	i_dtime;

	/*
	 * i_block_group is the number of the block group which contains
	 * this file's inode.  Constant across the lifetime of the inode,
	 * it is used for making block allocation decisions - we try to
	 * place a file's data blocks near its inode block, and new inodes
	 * near to their parent directory's inode.
	 */
	__u32	i_block_group;
	__u32	i_dir_start_lookup;

	struct inode	vfs_inode;
};
#define EXT2_STATE_NEW			0x00000001 /* inode is newly created */

static inline struct ext2_inode_info *EXT2_I(struct inode *inode) {
	return container_of(inode, struct ext2_inode_info, vfs_inode);
}

static inline struct ext2_sb_info *EXT2_SB(struct super_block *sb) {
	return (struct ext2_sb_info *) sb->s_fs_info;
}

extern struct inode* ext2_iget(struct super_block *sb, uint32_t ino);
extern struct dentry* ext2_lookup(struct inode *dir, struct dentry *child, unsigned int flags);
extern struct page *ext2_get_page(struct inode *inode, uint32_t index);
extern void ext2_put_page(struct page *page);

/* dir.c */
extern const struct file_operations ext2_dir_operations;

/* file.c */
extern const struct inode_operations ext2_file_inode_operations;
extern const struct file_operations ext2_file_operations;

/* inode.c */
extern const struct address_space_operations ext2_aops;

/* namei.c */
extern const struct inode_operations ext2_dir_inode_operations;
extern const struct inode_operations ext2_special_inode_operations;

/* symlink.c */
extern const struct inode_operations ext2_symlink_inode_operations;
/*

sudo mount -o loop rootfs.ext2 ./mnt
sudo umount /tmp/ext2mnt

*/
#endif