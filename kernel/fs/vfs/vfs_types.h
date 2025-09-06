/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_types.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 16:16:30
 * @LastEditTime: 2025-09-06 15:46:08
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#include "block_adapter.h"
#include "list.h"
#include "time.h"
#include "types.h"
#include "spinlock.h"
#include "lru.h"

typedef uint64_t vfs_ino_t;
typedef struct vfs_super_ops vfs_super_ops_t;
typedef struct vfs_inode_ops vfs_inode_ops_t;
typedef struct vfs_dentry_ops vfs_dentry_ops_t;
typedef struct vfs_file_ops vfs_file_ops_t;
typedef struct vfs_fs_type vfs_fs_type_t;
typedef struct vfs_superblock vfs_superblock_t;
typedef struct vfs_inode vfs_inode_t;
typedef struct vfs_file vfs_file_t;
typedef struct qstr qstr_t;
typedef struct vfs_dentry vfs_dentry_t;
typedef struct vfs_statfs vfs_statfs_t;
typedef struct vfs_mount vfs_mount_t;



struct vfs_fs_type
{
    const char *name;
    vfs_superblock_t* (*mount)(vfs_fs_type_t *fs_type, block_device_t *adap, int64_t flags);
    void (*kill_sb)(vfs_superblock_t *sb);
    vfs_super_ops_t *s_ops; 
    int64_t fs_flag;
    list_t sb_lhead;
    list_t fs_type_lnode;
};


struct vfs_super_ops
{
    void* (*create_private_inode)();
    int64_t (*new_private_inode)(vfs_inode_t * );
    int64_t (*read_inode)(vfs_inode_t *);
    int64_t (*write_inode)(vfs_inode_t *);
    int64_t (*sync_fs)(vfs_superblock_t *);
    int64_t (*statfs)(vfs_superblock_t *, vfs_statfs_t *);
};
struct vfs_statfs 
{
    uint64_t f_type;     // 文件系统类型
    uint64_t f_bsize;    // 块大小
    uint64_t f_blocks;   // 总数据块数
    uint64_t f_bfree;    // 空闲块数
    uint64_t f_bavail;   // 可用块数
    uint64_t f_files;    // 总 inode 数
    uint64_t f_ffree;    // 空闲 inode 数
    uint64_t f_namelen;  // 最大文件名长度
};

struct vfs_superblock
{
/* 超级块标志位 */
#define VFS_SB_ACTIVE     0x0001  // 超级块处于活动状态
#define VFS_SB_DIRTY      0x0002  // 超级块需要写回磁盘
#define VFS_SB_RDONLY     0x0004  // 只读文件系统
    list_t s_list;
    uint64_t s_block_size;
    vfs_fs_type_t *s_type;
    block_adapter_t *adap;
    const vfs_super_ops_t *s_ops;
    uint32_t s_magic;
    vfs_inode_t *s_root; // 根目录的inode
    vfs_statfs_t statfs;      // 获取文件系统统计信息
    void *s_private;
};



#define VFS_PAGE_SIZE 4096  
typedef uint64_t pgoff_t; // page index
typedef struct page 
{
    lru_node_t p_lru_cache_node;  //全局page lru缓存链表节点
    hlist_node_t self_cache_node; // 哈希表节点，用于快速查找inode私有的page缓存
    spinlock_t lock;          // page 锁（简化用 pthread_mutex）
    // pthread_cond_t  wait;          // 等待/唤醒
    bool under_io;           // 正在读/写磁盘
    bool uptodate;                  // 内容有效
    bool dirty;                     // 脏页标志
    int64_t refcount;                  // 引用计数
    vfs_inode_t *inode;            // 所属 inode
    pgoff_t index;                 // page index in file
    uint8_t *data;                 // 指向 PAGE_SIZE 内存
} vfs_page_t;


typedef struct vfs_aops 
{
    int64_t (*readpage)(vfs_page_t *page);
    int64_t (*writepage)(vfs_page_t *page);
}vfs_aops_t;

typedef struct vfs_address_space 
{
    vfs_inode_t *host;
    hashtable_t *page_cache;   
    const  vfs_aops_t *a_ops;
}vfs_address_space_t;

struct vfs_inode_ops
{
    int64_t (*lookup)(vfs_inode_t *dir,const char *name);
    int64_t (*mkdir)(vfs_inode_t *dir, const char *name, uint32_t i_mode); 
};
struct vfs_inode
{
/* inode 标志位 */
#define I_DIRTY       0x0001  // inode 脏，需要写回
#define I_NEW         0x0002  // 新分配的 inode，尚未初始化
#define I_FREEING     0x0004  // inode 正在被释放
    // 公共字段
    uint32_t i_ino;
    uint16_t i_mode;
    uint32_t i_size;
    uint16_t i_uid;
    uint16_t i_gid;
    uint32_t i_nlink; // 链接数
    timespec_t i_atime; // 最后访问时间
    timespec_t i_mtime; // 最后修改时间
    timespec_t i_ctime; // 最后状态改变时间

    // vfs字段
    uint32_t i_count; // 引用计数
    vfs_superblock_t *i_sb;
    const vfs_inode_ops_t *s_op;
    const vfs_file_ops_t *i_fop; // 文件操作函数表
    vfs_address_space_t *i_mapping; // 地址空间
    spinlock_t i_lock;          // 保护inode的锁
    lru_node_t i_lru_cache_node;            // 缓存节点，用于LRU算法管理
    uint16_t i_flags;              // inode 标志位
    bool dirty;               // 是否被修改，需要写回磁盘

    // FS 私有字段
    void *i_private;             // FS 私有 inode 数据
};



struct qstr
{
    const char *name; // 文件名
    uint32_t len;     // 文件名长度
};

struct vfs_dentry
{
    qstr_t name;
    vfs_inode_t *d_inode;   // 关联的inode
    vfs_dentry_t *d_parent; // 父目录项
    list_t d_childs;   // 子目录项链表
    lru_cache_t *d_lru_cache_node; // 目录项缓存节点
    int64_t d_refcount;    // 引用计数
    vfs_dentry_ops_t *d_op;
    // void *d_private;
};



struct vfs_file_ops
{
    int64_t (*read)(struct vfs_inode *inode, void *buf, size_t size, loff_t offset);
    int64_t (*write)(struct vfs_inode *inode, const void *buf, size_t size, loff_t offset);
    // int64_t (*truncate)(struct vfs_inode *inode, uint64_t size);
    // int64_t (*sync)(struct vfs_file *f); // 可选
};

struct vfs_file
{
    vfs_dentry_t *f_entry; // 关联的entry
    vfs_file_ops_t *f_fops; // 文件操作函数
    loff_t f_pos;          // 当前文件位置
    uint32_t f_flags;      // 文件打开标志
    void *f_private;       // 文件系统私有数据
};



struct vfs_mount
{
    vfs_superblock_t *mnt_sb; // 挂载的超级块
    vfs_dentry_t *mnt_root;   // 挂载点根目录
    list_t mnt_list;          // 链表节点
};




#endif