/**
 * @FilePath: /ZZZ-OS/include/fs/vfs_types.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 16:16:30
 * @LastEditTime: 2025-11-25 15:12:13
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#include <fs/block_adapter.h>
#include <os/list.h>
#include <os/lru.h>
#include <asm/spinlock.h>
#include <drivers/time.h>
#include <os/types.h>
#include <os/pfn.h>

typedef long long loff_t;
typedef int64_t ino_t;


struct superblock;
struct inode;
struct dentry;
struct file;
struct fs_type;
struct super_ops;
struct inode_ops;
struct file_ops;
struct statfs;

struct fs_type
{
    const char *name;
    struct superblock *(*mount)(struct fs_type *fs_type, struct block_device *adap, int flags);
    void (*kill_sb)(struct superblock *sb);
    struct super_ops *s_ops;
    int fs_flag;
    struct list_head sb_lhead;
    struct list_head fs_type_lnode;
};

struct super_ops
{
    void *(*create_private_inode)();
    int (*new_private_inode)(struct inode *);

    int (*read_inode)(struct inode *);
    int (*write_inode)(struct inode *);
    int (*sync_fs)(struct superblock *);
    int (*statfs)(struct superblock *, struct statfs *);
};
struct statfs
{
    uint64_t f_type;    // 文件系统类型
    uint64_t f_bsize;   // 块大小
    uint64_t f_blocks;  // 总数据块数
    uint64_t f_bfree;   // 空闲块数
    uint64_t f_bavail;  // 可用块数
    uint64_t f_files;   // 总 inode 数
    uint64_t f_ffree;   // 空闲 inode 数
    uint64_t f_namelen; // 最大文件名长度
};

struct superblock
{
/* 超级块标志位 */
#define VFS_SB_ACTIVE 0x0001 // 超级块处于活动状态
#define VFS_SB_DIRTY 0x0002  // 超级块需要写回磁盘
#define VFS_SB_RDONLY 0x0004 // 只读文件系统
    struct list_head s_list;
    uint64_t s_block_size;
    struct fs_type *s_type;
    struct block_adapter *adap;
    struct super_ops *s_ops;
    uint32_t s_magic;
    struct inode *s_root; // 根目录的inode
    struct statfs statfs; // 获取文件系统统计信息
    void *s_private;
};

#define VFS_PAGE_SIZE PAGE_SIZE
typedef uintptr_t pgoff_t; // page index

struct page_cache
{
    struct lru_node p_lru_cache_node;  // 全局page lru缓存链表节点
    struct hlist_node self_cache_node; // 哈希表节点，用于快速查找inode私有的page缓存
    struct spinlock lock;              // page 锁（简化用 pthread_mutex）
    // pthread_cond_t  wait;          // 等待/唤醒
    int refcount;   // 引用计数
    bool under_io;  // 正在读/写磁盘
    bool uptodate;  // 内容有效
    bool dirty;     // 脏页标志
    struct inode *inode; // 所属 inode
    pgoff_t index;  // page index in file
    char *data;       // 指向 PAGE_SIZE 内存
};

struct aops
{
    int (*readpage)(struct page_cache*page);
    int (*writepage)(struct page_cache*page);
};

struct address_space
{
    struct inode *host;
    struct hashtable *page_cache;
    struct aops *a_ops;
};

struct inode_ops
{
    int (*lookup)(struct inode *dir, struct dentry *dentry);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, uint32_t i_mode);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
    int (*creat)(struct inode *dir, struct dentry *dentry, uint32_t i_mode);
    int (*mknod)(struct inode *i_parent, struct dentry *dentry, uint32_t i_mode, dev_t devnr);
};
// 文件类型位掩码 注意这是8进制，不是16进制
#define S_IFMT 00170000  // 文件类型位掩码（八进制）
#define S_IFSOCK 0140000 // 套接字
#define S_IFLNK 0120000  // 符号链接
#define S_IFREG 0100000  // 普通文件
#define S_IFBLK 0060000  // 块设备
#define S_IFDIR 0040000  // 目录
#define S_IFCHR 0020000  // 字符设备
#define S_IFIFO 0010000  // FIFO（命名管道）

// 文件类型判断宏
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

// 设置用户ID、组ID和粘滞位
#define S_ISUID 0004000 // 设置用户ID
#define S_ISGID 0002000 // 设置组ID
#define S_ISVTX 0001000 // 粘滞位

// 用户权限
#define S_IRUSR 0000400 // 用户读权限
#define S_IWUSR 0000200 // 用户写权限
#define S_IXUSR 0000100 // 用户执行权限

// 组权限
#define S_IRGRP 0000040 // 组读权限
#define S_IWGRP 0000020 // 组写权限
#define S_IXGRP 0000010 // 组执行权限

// 其他用户权限
#define S_IROTH 0000004 // 其他用户读权限
#define S_IWOTH 0000002 // 其他用户写权限
#define S_IXOTH 0000001 // 其他用户执行权限

// 常用权限组合
#define S_IRWXU 0000700 // 用户读、写、执行权限
#define S_IRWXG 0000070 // 组读、写、执行权限
#define S_IRWXO 0000007 // 其他用户读、写、执行权限

#define S_IDEFAULT 0x01ed

struct inode
{
    // 公共字段
    uint32_t i_ino;
    uint16_t i_mode;
    uint32_t i_size;
    uint16_t i_uid;
    uint16_t i_gid;
    uint32_t i_nlink;        // 链接数
    dev_t i_rdev;      // 设备号（如果是设备文件）
    timespec_t i_atime; // 最后访问时间
    timespec_t i_mtime; // 最后修改时间
    timespec_t i_ctime; // 最后状态改变时间

    // vfs字段
    struct superblock *i_sb;
    struct inode_ops *i_ops;
    struct file_ops *f_ops;                // 文件操作函数表
    struct address_space *i_mapping;  // 地址空间
    spinlock_t i_lock;           // 保护inode的锁
    struct lru_node i_lru_cache_node; // 缓存节点，用于LRU算法管理
    uint16_t i_flags;                      // inode 标志位
    bool dirty;                       // 是否被修改，需要写回磁盘
    int i_refcount;                 // 引用计数

    // FS 私有字段
    void *i_private; // FS 私有 inode 数据
};

struct qstr
{
    char *name; // 文件名
    uint32_t len;    // 文件名长度
};

// dentry 状态标志
enum dentry_state
{
    DCACHE_REFERENCED = 1 << 0,     // 最近被访问过
    DCACHE_LRU_LIST = 1 << 1,       // 在 LRU 列表中
    DCACHE_DISCONNECTED = 1 << 2,   // 与父目录断开连接
    DCACHE_ENTRY_TYPE = 1 << 3,     // 条目类型已知
    DCACHE_MANAGED_DENTRY = 1 << 4, // 受管理的 dentry
    DCACHE_MOUNTED = 1 << 5,        // 挂载点
    DCACHE_NEED_AUTOMOUNT = 1 << 6, // 需要自动挂载
    DCACHE_DONTCACHE = 1 << 7,      // 不缓存
    DCACHE_DENTRY_KILLED = 1 << 8,  // 已被杀死
    DCACHE_NEGATIVE = 1 << 9,       // 负目录项
};
struct dentry
{
#define VFS_NAME_MAX 255
    struct qstr name;
    struct inode *d_inode;                 // 关联的inode
    struct dentry *d_parent;          // 父目录项
    struct list_head d_childs;             // 父目录的子目录链表节点
    struct list_head d_subdirs;            // 本目录的子目录项链表头
    struct lru_node d_lru_cache_node; // 目录项缓存
    spinlock_t d_lock;
    enum dentry_state d_flags; // dentry状态标志
    struct dentry_ops *d_op;
    int d_refcount; // 引用计数
    // void *d_private;
};

struct file_ops
{
    int (*open) (struct inode *, struct file *);
    ssize_t (*read)(struct inode*, void*, size_t, loff_t*);
    ssize_t (*write)(struct inode*, const void*, size_t, loff_t*);
};

struct file
{
    enum { FD_NONE=0, FD_FILE, FD_DIR, FD_DEVICE } type;
    struct inode *f_inode;        // 指向文件对应的 inode
    struct dentry *f_dentry; // 打开的 dentry (路径)
    loff_t f_pos;            // 文件读写偏移
    uint32_t f_flags;             // 打开模式 (O_RDONLY, O_WRONLY, O_RDWR, O_CREAT...)
    uint32_t f_mode;              // 权限（一般拷贝自inode->i_mode，但可在open时限制）
    int f_refcount;          // 引用计数 (多少进程共享这个 file)
    void *private_data;      // 私有数据，具体fs可以存东西
};

struct mount_point
{
    struct superblock *mnt_sb; // 挂载的超级块
    struct inode *mnt_root;   // 挂载点根目录
    struct list_head mnt_list;      // 链表节点
};

struct path
{
    struct vfsmount *mnt;  // 挂载点信息
    struct dentry *dentry; // 目录项
};

#endif