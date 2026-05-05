# 文件系统

[TOC]

## 1. VFS 概述

ZZZ-OS 的 VFS（Virtual File System）层提供统一的文件系统抽象，支持多种文件系统的挂载和访问。设计参考 Linux VFS，包含 dentry 缓存、inode 缓存、page cache 和 mount namespace。

核心组件：
- **dentry**：目录项缓存，加速路径查找
- **inode**：文件元数据和操作抽象
- **superblock**：每个挂载点的文件系统根信息
- **file**：打开的文件描述符
- **mount/namespace**：挂载树管理
- **page cache**：文件数据缓存

## 2. 文件系统注册

位于 `fs/registry.c`。每种文件系统提供 `struct file_system_type` 实例：

```c
struct file_system_type {
    u32 sb_flags;
    u32 purpose;
    const char *source;
    void *fs_private;
    struct dentry *root;
    // ...
};
```

`register_filesystem()` 将 `file_system_type` 挂到全局链表 `g_filesystems` 上。这使得内核可以知道目前有哪些文件系统可用，不过也仅限于此。只有真正挂载之后，文件系统才可以被内核使用。

## 3. 文件系统挂载

挂载过程主要分三个层次：

### 3.1 第一层：挂载上下文创建

根据注册的 `file_system_type` 创建挂载上下文（mount context），`s_context_for_mount()` 为某个 `file_system_type` 分配并初始化上下文。

### 3.2 第二层：文件系统实现

`vfs_get_tree()` 调用 `fs_type->get_tree(fc)`，让具体文件系统把这次挂载对应的根 dentry 准备好，放入 `fc->root`。

### 3.3 第三层：挂载树 / namespace

位于 `fs/namespace.c`：

- `vfs_kern_mount()` 把 `fc->root` 包成一个 `struct vfsmount`
- `init_mount_tree()` 把这个 mount 安装成当前系统的 root mount

### 3.4 完整流程

串联整个流程的是 `fs/fs2_init.c`：

```
register_filesystem(&ramfs_fs_type)     // 注册 ramfs
  → fs_context_for_mount()              // 创建挂载上下文
  → vfs_get_tree(fc)                    // 获取文件系统根 dentry
  → vfs_kern_mount(fc, &root_mnt)       // 创建 vfsmount
  → init_mount_tree(root_mnt)           // 安装为根挂载
```

根文件系统先挂载为 ramfs，然后通过 `mount_root()` 将 ext2 挂载为实际根文件系统。

## 4. 已实现的文件系统

### 4.1 ramfs

位于 `fs/ramfs/`。纯内存文件系统，用作初始根文件系统（initramfs 角色）。

### 4.2 ext2

位于 `fs/ext2/`。完整的 ext2 文件系统实现，支持读写操作：
- 块分配与释放
- 目录项管理
- 文件读写
- inode 管理
- superblock 解析

## 5. 路径查找（namei）

路径查找通过 `namei.c` 实现，支持：
- 绝对路径和相对路径解析
- `.` 和 `..` 特殊目录
- 符号链接跟随（待实现）

查找结果返回 `dentry` 和对应的 `vfsmount`。

## 6. Page Cache

位于 `fs/pagecache.c`。page cache 为文件数据提供缓存层：
- 按需从块设备读取页面
- 写回脏页面到块设备
- LRU 管理（通过 `lib/lru.c`）

## 7. 设备文件

### 7.1 字符设备（cdev）

通过 `fs/cdev.c` 管理。字符设备通过 `devnode_register()` 注册到 VFS，提供 `file_operations` 接口。

### 7.2 块设备（blkdev）

通过 `fs/blkdev.c` 管理。块设备提供底层存储接口，被 ext2 等文件系统使用。

## 8. 目录操作

`fs/readdir.c` 实现 `getdents` 系统调用，用于读取目录内容。返回 `struct dirent` 列表。
