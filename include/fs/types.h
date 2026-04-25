#ifndef FS2_TYPES_H
#define FS2_TYPES_H

#include <os/list.h>
#include <os/spinlock.h>
#include <os/timekeeping.h>
#include <os/types.h>
#include <os/lru.h>
#include <os/bitops.h>

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

typedef long long loff_t;
typedef int32_t ino_t;
typedef uintptr_t pgoff_t;

struct file_system_type;
struct fs_context;
struct super_block;
struct inode;
struct dentry;
struct file;
struct page;
struct vfsmount;
struct path;
struct super_operations;
struct inode_operations;
struct file_operations;
struct address_space;
struct address_space_operations;
struct block_device;

struct qstr {
    char *name;
    uint16_t len;
};

struct super_operations {
    void (*put_super)(struct super_block *sb);
    int (*write_inode)(struct inode *inode);
    // 文件系统同步超级块
    int (*sync_fs)(struct super_block *sb);

    // 文件系统建立一个新的私有inode(但并不会从磁盘填充具体数据)并返回嵌入其中的通用inode，供VFS使用
    struct inode *(*alloc_inode)(struct super_block *sb); 
    void (*destroy_inode)(struct inode *inode);
};

struct inode_operations {
    struct dentry * (*lookup) (struct inode *dir,struct dentry *, unsigned int);
    int (*create)(struct inode *dir, struct dentry *dentry, uint16_t mode);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, uint16_t mode);
    int (*mknod)(struct inode *dir, struct dentry *dentry, uint16_t mode, dev_t dev);
};

struct file_operations {
    int (*open)(struct inode *inode, struct file *file);
    ssize_t (*read)(struct file *file, char *buf, size_t len, loff_t *ppos);
    ssize_t (*write)(struct file *file, const char *buf, size_t len, loff_t *ppos);
};

struct file_system_type {
    const char *name;
    int (*init_fs_context)(struct fs_context *fc);
    int (*get_tree)(struct fs_context *fc); // 通过 fs_context 构建 super_block 和根目录 dentry
    void (*kill_sb)(struct super_block *sb);
    struct list_head fs_list;
};

struct super_block {
    uint32_t s_magic;
    uint32_t s_blocksize;
    uint32_t s_flags;
    struct file_system_type *s_type;
    const struct super_operations *s_op;
    struct dentry *s_root;
    struct block_device *s_bdev;
    void *s_fs_info;
    spinlock_t s_lock;
    int s_active;
    struct list_head s_instances;
};

struct address_space_operations {
    int (*readpage)(struct page *page);
    int (*writepage)(struct page *page);
};

struct address_space {
    struct inode *host;
    const struct address_space_operations *a_ops;
    spinlock_t lock;
    uint32_t nrpages;
};

#define I_NEW 0x00 // inode 是新创建的，还没有被填充数据
#define I_UPTODATE 0x01 // inode 的数据已经从磁盘加载到内存
#define I_DIRTY 0x02 // inode 已经被修改，需要写回磁盘

struct inode {
    ino_t i_ino;
    uint16_t i_mode;
    uint32_t i_nlink;
    size_t i_size;
    dev_t i_rdev;
    timespec_t i_atime;
    timespec_t i_mtime;
    timespec_t i_ctime;

    struct super_block *i_sb;
    const struct inode_operations *i_op;
    const struct file_operations *i_fop;
    struct address_space *i_mapping;
    struct address_space i_data;
    spinlock_t i_lock;
    int i_count;
    uint8_t i_state;

    struct lru_node d_lru_cache_node; // inode 缓存
    void *i_private;
};

struct dentry {
    struct qstr d_name;
    struct inode *d_inode;
    struct dentry *d_parent;
    struct super_block *d_sb;
    spinlock_t d_lock;
    int d_count;
    unsigned int d_flags;
    struct list_head d_child;
    struct list_head d_subdirs;

    struct lru_node d_lru_cache_node; // 目录项缓存
    void *d_fsdata;
};

struct vfsmount {
    spinlock_t lock;
    struct super_block *mnt_sb;
    struct dentry *mnt_root;
    struct dentry *mnt_mountpoint;
    int refcount;
    struct list_head mnt_list;
};

struct fs_context {
    struct file_system_type *fs_type;
    uint32_t sb_flags;
    uint32_t purpose;
    const char *source;
    void *fs_private;
    struct dentry *root;
};

struct path {
    struct vfsmount *mnt;
    struct dentry *dentry;
};

struct file {
    struct path f_path;
    struct inode *f_inode;
    const struct file_operations *f_op;
    loff_t f_pos;
    uint32_t f_flags;
    atomic_t f_count;
    void *private_data;
};

#define MAX_OPEN_FILES_NUM 64

struct fdtable {
    int max_fds; // 最多能开多少文件
    unsigned long *open_fds; // 位图，标记哪个fd已经使用
    unsigned long *close_on_exec;// 位图，exec时要关掉的文件
    struct file **fd; // 数组：fd号 对应打开的文件
};

static inline bool close_on_exec(int fd, const struct fdtable *fdt) {
	return test_bit(fd, fdt->close_on_exec);
}

static inline bool fd_is_open(int fd, const struct fdtable *fdt) {
	return test_bit(fd, fdt->open_fds);
}


struct files_struct {
    atomic_t refcount;
    spinlock_t file_lock;
    struct fdtable fdtab;
    int next_fd;

    // 由于struct fdtable里的一些成员（例如位图）都是指针，用到时候需要给他们分配地址，不如直接在这个结构体里分配给它
    unsigned long close_on_exec_init[1];  // exec时要关掉的文件
    unsigned long open_fds_init[1];       // 哪些抽屉正在用
    struct file *fd_array[MAX_OPEN_FILES_NUM];
};

struct fs_struct {
    int users; // 多少个进程共享这个目录环境
    int umask;              // 创建新文件/目录时的默认权限掩码
	int in_exec;            // 进程正在执行 exec 吗？
    spinlock_t lock;        
    struct path root, pwd;
};


#endif
