# 文件系统
[TOC]
## 1.VFS
### 1.1 文件系统注册
位于[fs/register.c](../fs/registry.c)中。每种文件系统的实例都需要提供自己的`struct file_system_type`实现，`register_filesystem()`会将`struct file_system_type`挂到全局链表 `g_filesystems`上。这使得内核可以知道目前有哪些文件系统可用，不过也仅限于此。只有真正挂载之后，文件系统才可以被内核使用。
### 1.2 文件系统挂载
挂载过程主要分三个部分:

第一层首先是根据上面的创建一个挂载上下文。在文件系统注册时，我们有了`struct file_system_type`，在这里，`s_context_for_mount()`会给某个 `file_system_type` 分配一个 
```c
struct file_system_type {
    uint32_t sb_flags;
    uint32_t purpose;
    const char *source;
    void *fs_private;
    struct dentry *root;
}
```

第二层是具体文件系统实现。`vfs_get_tree()`调用 `fs_type->get_tree(fc)`，让具体文件系统把这次挂载对应的根 dentry 准备好，放进 `fc->root`。

第三层是挂载树/namespace，在 [fs/namespace.c](../fs/namespace.c)：

- `vfs_kern_mount()`把`fc->root`包成一个`struct vfsmount`
- `init_mount_tree()`把这个 mount 安装成当前系统的 root mount

最后把整条流程串起来是 [fs/fs_init.c](../fs/fs2_init.c)：

1. register_filesystem(&ramfs_fs_type)
2. fs_context_for_mount(&ramfs_fs_type, 0)
3. vfs_get_tree(fc)
4. vfs_kern_mount(fc, &root_mnt)
5. init_mount_tree(root_mnt)