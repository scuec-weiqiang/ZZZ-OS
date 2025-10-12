#ifndef EXT2_H
#define EXT2_H

#include "types.h"
#include "bitmap.h"
#include "vfs_types.h"

typedef u32 ext2_ino_t;
typedef u32 ext2_bno_t;

struct __attribute__((packed)) ext2_superblock
{
#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPERBLOCK_SIZE   1024
#define EXT2_SUPER_MAGIC 0xEF53
    u32 s_inodes_count;      // inode数
    u32 s_blocks_count;      // 块数
    u32 s_r_blocks_count;    // 保留块数
    u32 s_free_blocks_count; // 空闲块数
    u32 s_free_inodes_count; // 空闲inode数
    u32 s_first_data_block;  // 第一个数据块索引(通常是1 for 1k block, 0 for >1k)
    u32 s_log_block_size;    // 块大小的对数值,实际块大小=(1024 << s_log_block_size)
    u32 s_log_frag_size;     // 碎片大小的对数值，同上
    u32 s_blocks_per_group;  // 每个组的块数
    u32 s_frags_per_group;   // 每个块组多少碎片（已废弃）
    u32 s_inodes_per_group;  // 每个组的inode数
    u32 s_mtime;             // 最后挂载时间
    u32 s_wtime;             // 最后写入时间
    u16 s_mnt_count;         // 挂载次数
    u16 s_max_mnt_count;     // 最大挂载次数
    u16 s_magic;             // 文件系统魔数，必须是0xEF53
    u16 s_state;             // 文件系统状态
    u16 s_errors;            // 错误处理策略
    u16 s_minor_rev_level;   // 次版本号
    u32 s_lastcheck;         // 最后检查时间
    u32 s_checkinterval;     // 检查间隔
    u32 s_creator_os;        // 创建 OS
    u32 s_rev_level;         // 修订版本
    u16 s_def_resuid;        // 默认保留 uid
    u16 s_def_resgid;        // 默认保留 gid
    // ... 其他字段
    u32 s_first_ino;              /* First non-reserved inode */
    u16 s_inode_size;             /* size of inode structure */
    u16 s_block_group_nr;         /* block group # of this superblock */
    u32 s_feature_compat;         /* compatible feature set */
    u32 s_feature_incompat;       /* incompatible feature set */
    u32 s_feature_ro_compat;      /* readonly-compatible feature set */
    u8 s_uuid[16];                /* 128-bit uuid for volume */
    char s_volume_name[16];            /* volume name */
    char s_last_mounted[64];           /* directory where last mounted */
    u32 s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    u8 s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    u8 s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    u16 s_padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    u8 s_journal_uuid[16]; /* uuid of journal superblock */
    u32 s_journal_inum;    /* inode number of journal file */
    u32 s_journal_dev;     /* device number of journal file */
    u32 s_last_orphan;     /* start of list of inodes to delete */
    u32 s_hash_seed[4];    /* HTREE hash seed */
    u8 s_def_hash_version; /* Default hash version to use */
    u8 s_reserved_char_pad;
    u16 s_reserved_word_pad;
    u32 s_default_mount_opts;
    u32 s_first_meta_bg; /* First metablock block group */
    u32 s_reserved[190]; /* Padding to the end of the block */
};

struct __attribute__((packed)) ext2_groupdesc
{
    u32 bg_block_bitmap;      // 块位图的块号
    u32 bg_inode_bitmap;      // inode位图的块号
    u32 bg_inode_table;       // inode表的起始块号
    u16 bg_free_blocks_count; // 组内空闲块数
    u16 bg_free_inodes_count; // 组内空闲inode数
    u16 bg_used_dirs_count;   // 组内目录数
    u16 bg_pad;
    u32 bg_reserved[3];
};

#define EXT2_GET_TYPE(x) ((x) & 0xF000)

struct __attribute__((packed)) ext2_inode 
{
#define EXT2_ROOT_INODE_IDX 2 //根目录的编号
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

    u16 i_mode;        // 文件类型和权限
    u16 i_uid;         // 用户ID
    u32 i_size;        // 文件大小（字节）
    u32 i_atime;       // 最后访问时间
    u32 i_ctime;       // 创建时间
    u32 i_mtime;       // 最后修改时间
    u32 i_dtime;       // 删除时间
    u16 i_gid;         // 组ID
    u16 i_links_count; // 硬链接计数
    u32 i_blocks;      // 文件占用块数（以512字节为单位）
    u32 i_flags;       // 文件标志
    union
    {
        struct
        {
            u32 l_i_reserved1;
        } linux1;
        struct
        {
            u32 h_i_translator;
        } hurd1;
        struct
        {
            u32 m_i_reserved1;
        } masix1;
    } osd1; /* OS dependent 1 */ // 保留字段
    u32 i_block[15];   // 文件占据的块号（12直接，1个间接，1个双重间接，1个三重间接）
    u32 i_generation;  // 文件版本
    u32 i_file_acl;    // ACL（稀有）
    u32 i_dir_acl;     // 目录 ACL
    u32 i_faddr;       // 碎片地址
    union {
		struct {
			u8	l_i_frag;	/* Fragment number */
			u8	l_i_fsize;	/* Fragment size */
			u16	i_pad1;
			u16	l_i_uid_high;	/* these 2 fields    */
			u16	l_i_gid_high;	/* were reserved2[0] */
			u32	l_i_reserved2;
		} linux2;
		struct {
			u8	h_i_frag;	/* Fragment number */
			u8	h_i_fsize;	/* Fragment size */
			u16	h_i_mode_high;
			u16	h_i_uid_high;
			u16	h_i_gid_high;
			u32	h_i_author;
		} hurd2;
		struct {
			u8	m_i_frag;	/* Fragment number */
			u8	m_i_fsize;	/* Fragment size */
			u16	m_pad1;
			u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

struct __attribute__((packed)) ext2_dir_entry_2
{
#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7

    u32 inode;    // inode号 (0表示该条目未使用)
    u16 rec_len;  // 该目录项的总长度
    u8 name_len;  // 文件名长度
    u8 file_type; // 文件类型
    char name[];       // 文件名
};

struct ext2_fs_info
{
    u32 s_blocks_per_group;
    u32 s_inodes_per_group;
    u32 s_itb_per_group;     // 一个group中的inode table占的block数
    u32 s_ibmb_per_group; // 每个group的inode位图占的block数
    u32 s_bbmb_per_group; // 每个group的block位图占的block数

    u32 s_inodes_per_block;
    u32 s_desc_per_block;    // 一个block中能放多少个group descripter

    u32 s_gdb_count;         // 所有group descripter占的block数
    u32 s_groups_count;      // 有多少块组
    u32 s_inodes_count;      // inode数
    u32 s_blocks_count;      // 块数
    u32 s_free_blocks_count; // 空闲块数
    u32 s_free_inodes_count; // 空闲inode数

    u32 s_first_data_block;  // 第一个数据块索引(通常是1 for 1k block, 0 for >1k)
    struct ext2_superblock *super;
    struct ext2_groupdesc *group_desc;

    // 缓存
    struct
    {
        struct bitmap *ibm;
        int cached_group; // 当前缓存的块组
        bool dirty;      // inode位图是否被修改
    } ibm_cache;
    struct
    {
        struct bitmap *bbm;
        int cached_group; // 当前缓存的块组
        bool dirty;      // 块位图是否被修改
    } bbm_cache;
    struct
    {
        struct ext2_inode *it;
        int cached_group; // 当前缓存的块组
        bool dirty;      // inode表是否被修改
    } it_cache;
    // struct ext2_inode *inode_table;
    u64 disk_size; // 磁盘大小（字节）
};

static inline struct ext2_inode* EXT2_INODE(struct inode *inode)
{
    return (struct ext2_inode *)inode->i_private;
}

static inline struct ext2_fs_info* EXT2_FS_INFO(struct superblock *sb)
{
    return (struct ext2_fs_info *)sb->s_private;
}

#endif