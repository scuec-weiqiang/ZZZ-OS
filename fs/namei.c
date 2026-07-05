#include <fs/namei.h>
#include <fs/dcache.h>
#include <fs/fs_struct.h>
#include <fs/namespace.h>
#include <os/check.h>
#include <os/types.h>
#include <os/check.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <fs/inode.h>
#include <os/sched.h>
#include <os/err.h>

struct path_cursor {
    const char *path;
    u32 pos;
};

static int qstr_is_dot(const struct qstr *name) {
    return name->len == 1 && name->name[0] == '.';
}

static int qstr_is_dotdot(const struct qstr *name) {
    return name->len == 2 && name->name[0] == '.' && name->name[1] == '.';
}

static void path_cursor_init(const char *path, struct path_cursor *cur) {
    cur->path = path;
    cur->pos = (path[0] == '/') ? 1 : 0;
}

static int path_is_equal(const struct path *a, const struct path *b) {
    return a->mnt == b->mnt && a->dentry == b->dentry;
}

static int path_init_start(const char *path, struct path *start) {
    if (path[0] == '/') {
        *start = current->fs->root;
    } else {
        *start = current->fs->pwd;
    }

    if (start->mnt == NULL || start->dentry == NULL) {
        return -ENOENT;
    }

    path_get(start);
    return 0;
}

static int path_step_down(struct path *path, const struct qstr *name) {
    struct dentry *next = NULL;
    struct dentry *cached = NULL;
    struct dentry *lookup = NULL;
    struct vfsmount *mounted = NULL;
    struct path new_path = {0};

    cached = d_lookup(path->dentry, name);
    if (cached != NULL) {
        next = dget(cached);
    } else {
        lookup = d_alloc_qstr(path->dentry, name);
        if (lookup == NULL) {
            return -ENOMEM;
        }
        next = path->dentry->d_inode->i_op->lookup(path->dentry->d_inode, lookup, 0);
    }

    if (IS_ERR(next)) {
        return PTR_ERR(next);
    }
    if (next == NULL) {
        return -ENOENT;
    }
    if (next->d_inode == NULL) {
        dput(next);
        return -ENOENT;
    }
    #define PATH_MAX 4096
    
    if (vfs_search_mount(next, &mounted) == 0) {
        new_path.mnt = mntget(mounted);
        new_path.dentry = dget(mounted->mnt_root);
        dput(next);
    } else if ((next->d_inode->i_mode & S_IFMT) == S_IFLNK) {
        // 处理符号链接
        char *link_target = kmalloc(PATH_MAX);
        if (link_target == NULL) {
            dput(next);
            return -ENOMEM;
        }
        ssize_t len = next->d_inode->i_op->readlink(next->d_inode, link_target, PATH_MAX);
        if (len < 0) {
            kfree(link_target);
            dput(next);
            return len;
        }
        link_target[len] = '\0';

        struct path link_path = {0};
        int ret = path_lookup(link_target, &link_path);
        kfree(link_target);
        dput(next);
        if (ret < 0) {
            return ret;
        }

        new_path.mnt = link_path.mnt;
        new_path.dentry = link_path.dentry;
    } else {
        new_path.mnt = mntget(path->mnt);
        new_path.dentry = next;
    }
    
    path_put(path);
    
    *path = new_path;
    return 0;
}

static struct dentry *prepare_child_dentry(struct dentry *parent,
                                           const struct qstr *name)
{
    struct dentry *cached;
    struct dentry *child;

    cached = d_lookup(parent, name);
    if (cached != NULL) {
        child = dget(cached);
        if (child->d_inode != NULL) {
            dput(child);
            return NULL;
        }
        return child;
    }

    return d_alloc_qstr(parent, name);
}

static int path_step_up(struct path *path, const struct path *root) {
    struct path new_path = {0};
    int ret;

    if (path_is_equal(path, root)) {
        return 0;
    }

    if (path->dentry == path->mnt->mnt_root) {
        ret = vfs_get_mnt_parent(path->mnt, &new_path);
        if (ret < 0) {
            return ret;
        }
    } else {
        new_path.mnt = mntget(path->mnt);
        new_path.dentry = dget(path->dentry->d_parent);
    }

    path_put(path);
    *path = new_path;
    return 0;
}

/**
 * @brief 获取路径中的下一个组件
 *
 * 该函数用于从路径游标中提取下一个组件（文件或目录名）。
 *
 * @param cur 指向路径游标的指针
 * @param name 指向用于存储组件名称的qstr结构体的指针
 *
 * @return 返回0表示成功，返回-1表示失败，返回1表示没有更多组件
 */
int next_component(struct path_cursor *cur, struct qstr *name) {
    u32 start;
    u32 end;

    if (cur == NULL || name == NULL || cur->path == NULL) {
        return -1;
    }

    while (cur->path[cur->pos] == '/') {
        cur->pos++;
    }

    if (cur->path[cur->pos] == '\0') {
        return 1;
    }

    start = cur->pos;
    end = start;

    while (cur->path[end] != '\0' && cur->path[end] != '/') {
        end++;
    }

    name->name = (char *)&cur->path[start];
    name->len = end - start;
    cur->pos = end;
    return 0;
}

void path_get(const struct path *path) {
	mntget(path->mnt);
	dget(path->dentry);
}

void path_put(const struct path *path) {
	mntput(path->mnt);
	dput(path->dentry);
}

int path_lookup(const char *path, struct path *out) {
    struct path_cursor cur;
    struct path current_path = {0};
    struct qstr name;
    int ret;

    CHECK(path != NULL, "fs: invalid path", return -EINVAL;);
    CHECK(out != NULL, "fs: invalid lookup output", return -EINVAL;);
    if (path[0] == '\0')
        return -ENOENT;

    // ret = path_init_start(path, &root);
    // if (ret < 0) {
    //     return ret;
    // }

    ret = path_init_start(path, &current_path);
    if (ret < 0) {
        return ret;
    }

    path_cursor_init(path, &cur);

    while (1) {
        ret = next_component(&cur, &name);
        if (ret < 0) {
            ret = -EFAULT;
            goto err_out;
        }

        if (ret > 0) {
            break;
        }

        if (qstr_is_dot(&name)) {
            continue;
        }

        if (qstr_is_dotdot(&name)) {
            ret = path_step_up(&current_path, &current->fs->root);
        } else {
            // dprintk("lookup component '%s' under dentry '%s'\n", name.name, current_path.dentry->d_name.name);
            ret = path_step_down(&current_path, &name);
        }
        if (ret < 0) {
            goto err_out;
        }
    }

    *out = current_path;
    return 0;

err_out:
    path_put(&current_path);
    out->mnt = NULL;
    out->dentry = NULL;
    return ret;
}

// 忽略软链接
int path_lookup_nofollow(const char *path, struct path *out) {
    struct path parent = {0};
    struct qstr last = {0};
    struct dentry *cached = NULL;
    struct dentry *lookup = NULL;
    struct dentry *dentry = NULL;
    int ret;

    CHECK(path != NULL, "fs: invalid path", return -EINVAL;);
    CHECK(out != NULL, "fs: invalid lookup output", return -EINVAL;);
    if (path[0] == '\0')
        return -ENOENT;

    ret = path_parentat(path, &parent, &last);
    if (ret < 0)
        return ret;

    if (qstr_is_dot(&last)) {
        out->mnt = parent.mnt;
        out->dentry = parent.dentry;
        return 0;
    }

    if (qstr_is_dotdot(&last)) {
        ret = path_step_up(&parent, &current->fs->root);
        if (ret < 0)
            goto err_put_parent;

        out->mnt = parent.mnt;
        out->dentry = parent.dentry;
        return 0;
    }

    cached = d_lookup(parent.dentry, &last);
    if (cached != NULL) {
        dentry = dget(cached);
    } else {
        lookup = d_alloc_qstr(parent.dentry, &last);
        if (lookup == NULL) {
            ret = -ENOMEM;
            goto err_put_parent;
        }

        dentry = parent.dentry->d_inode->i_op->lookup(parent.dentry->d_inode,
                                                      lookup, 0);
    }

    if (IS_ERR(dentry)) {
        ret = PTR_ERR(dentry);
        goto err_put_parent;
    }

    if (dentry == NULL || dentry->d_inode == NULL) {
        if (dentry != NULL)
            dput(dentry);
        ret = -ENOENT;
        goto err_put_parent;
    }

    out->mnt = parent.mnt;
    out->dentry = dentry;
    return 0;

err_put_parent:
    path_put(&parent);
    out->mnt = NULL;
    out->dentry = NULL;
    return ret;
}

int path_parentat(const char *path, struct path *parent, struct qstr *last) {
    struct path_cursor cur;
    // struct path root = {0};
    struct path current_path = {0};
    struct qstr name;
    struct qstr next_name;
    int ret;

    if (path == NULL || parent == NULL || last == NULL) {
        return -EINVAL;
    }
    if (path[0] == '\0') {
        return -EINVAL;
    }

    // ret = path_init_start(path, &root);
    // if (ret < 0) {
    //     return ret;
    // }

    ret = path_init_start(path, &current_path);
    if (ret < 0) {
        // path_put(&root);
        return ret;
    }

    path_cursor_init(path, &cur);

    while (1) {
        struct path_cursor save;
        int next_ret;

        ret = next_component(&cur, &name);
        if (ret < 0) {
            ret = -EFAULT;
            goto err_out;
        }

        if (ret > 0) {
            parent->mnt = mntget(current_path.mnt);
            parent->dentry = dget(current_path.dentry);
            last->name = current_path.dentry->d_name.name;
            last->len = current_path.dentry->d_name.len;
            path_put(&current_path);
            // path_put(&root);
            return 0;
        }

        save = cur;
        next_ret = next_component(&save, &next_name);
        if (next_ret > 0) {
            *parent = current_path;
            *last = name;
            // path_put(&root);
            return 0;
        }

        if (qstr_is_dot(&name)) {
            continue;
        }

        if (qstr_is_dotdot(&name)) {
            ret = path_step_up(&current_path, &current->fs->root);
        } else {
            ret = path_step_down(&current_path, &name);
        }
        if (ret < 0) {
            goto err_out;
        }
    }

err_out:
    path_put(&current_path);
    // path_put(&root);
    parent->mnt = NULL;
    parent->dentry = NULL;
    last->name = NULL;
    last->len = 0;
    return ret;
}

struct dentry* vfs_lookup(const char* path) {
    CHECK(path != NULL, "", return NULL;);

    struct path resolved;
    int ret = path_lookup(path, &resolved);
    CHECK(ret == 0, "vfs: path lookup failed", return NULL;);
    mntput(resolved.mnt);
    return resolved.dentry;
}

int vfs_mkdir(const char* path,u16 mode) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry;
    int ret;

    if (path == NULL || path[0] == '\0')
        return -ENOENT;

    ret = path_parentat(path, &resolved_parent, &child_name);
    if (ret < 0)
        return ret;

    child_dentry = prepare_child_dentry(resolved_parent.dentry, &child_name);
    if (child_dentry == NULL) {
        path_put(&resolved_parent);
        return -EEXIST;
    }

    ret = resolved_parent.dentry->d_inode->i_op->mkdir(resolved_parent.dentry->d_inode, child_dentry, mode);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return ret;
    }
    return 0;
}

struct dentry *vfs_create(const char *path, u16 mode) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    if (path == NULL || path[0] == '\0')
        return NULL;

    ret = path_parentat(path, &resolved_parent, &child_name);
    if (ret < 0)
        return NULL;

    child_dentry = prepare_child_dentry(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    ret = resolved_parent.dentry->d_inode->i_op->create(resolved_parent.dentry->d_inode, child_dentry, mode);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
}

struct dentry *vfs_mknod(const char *path, u16 mode, dev_t dev) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    CHECK(path != NULL, "", return NULL;);

    ret = path_parentat(path, &resolved_parent, &child_name);
    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    child_dentry = prepare_child_dentry(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    ret = resolved_parent.dentry->d_inode->i_op->mknod(resolved_parent.dentry->d_inode, child_dentry, mode, dev);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
}

struct dentry *vfs_symlink(const char *path, const char *target) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    CHECK(path != NULL && target != NULL, "", return NULL;);

    ret = path_parentat(path, &resolved_parent, &child_name);
    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    if (resolved_parent.dentry == NULL ||
        resolved_parent.dentry->d_inode == NULL ||
        resolved_parent.dentry->d_inode->i_op == NULL ||
        resolved_parent.dentry->d_inode->i_op->symlink == NULL) {
        path_put(&resolved_parent);
        return NULL;
    }

    child_dentry = prepare_child_dentry(resolved_parent.dentry, &child_name);
    if (child_dentry == NULL) {
        path_put(&resolved_parent);
        return NULL;
    }

    ret = resolved_parent.dentry->d_inode->i_op->symlink(
        resolved_parent.dentry->d_inode, child_dentry, target);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
}

ssize_t vfs_readlink(const char *path, char *buf, size_t size) {
    struct path resolved = {0};
    struct inode *inode;
    ssize_t ret;

    CHECK(path != NULL && buf != NULL, "fs: invalid readlink args", return -EINVAL;);

    if (size == 0)
        return 0;

    ret = path_lookup_nofollow(path, &resolved);
    if (ret < 0)
        return ret;

    inode = resolved.dentry->d_inode;
    if (inode == NULL) {
        ret = -ENOENT;
        goto out_put_path;
    }

    if (!S_ISLNK(inode->i_mode)) {
        ret = -EINVAL;
        goto out_put_path;
    }

    if (inode->i_op == NULL || inode->i_op->readlink == NULL) {
        ret = -EINVAL;
        goto out_put_path;
    }

    ret = inode->i_op->readlink(inode, buf, size);

out_put_path:
    path_put(&resolved);
    return ret;
}

int vfs_chdir(const char *path) {
    struct path resolved = {0};
    int ret;

    CHECK(path != NULL, "fs: invalid chdir path", return -EINVAL;);

    ret = path_lookup(path, &resolved);
    if (ret < 0) {
        return ret;
    }

    if (resolved.dentry == NULL || resolved.dentry->d_inode == NULL) {
        path_put(&resolved);
        return -ENOENT;
    }

    if (!S_ISDIR(resolved.dentry->d_inode->i_mode)) {
        path_put(&resolved);
        return -ENOTDIR;
    }

    set_fs_pwd(current->fs, &resolved);
    path_put(&resolved);
    return 0;
}

int vfs_unlink(const char *path) {
    struct path resolved = {0};
    struct path parent = {0};
    struct qstr last = {0};
    int ret;

    CHECK(path != NULL, "fs: invalid unlink path", return -EINVAL;);

    ret = path_lookup_nofollow(path, &resolved);
    if (ret < 0) {
        return ret;
    }

    if (resolved.dentry == resolved.mnt->mnt_root) {
        ret = -EBUSY;
        goto out_put_resolved;
    }

    if (resolved.dentry->d_inode == NULL) {
        ret = -ENOENT;
        goto out_put_resolved;
    }

    if (S_ISDIR(resolved.dentry->d_inode->i_mode)) {
        ret = -EISDIR;
        goto out_put_resolved;
    }

    ret = path_parentat(path, &parent, &last);
    if (ret < 0) {
        goto out_put_resolved;
    }

    if (parent.dentry == NULL || parent.dentry->d_inode == NULL ||
        parent.dentry->d_inode->i_op == NULL ||
        parent.dentry->d_inode->i_op->unlink == NULL) {
        ret = -EPERM;
        goto out_put_parent;
    }

    ret = parent.dentry->d_inode->i_op->unlink(parent.dentry->d_inode, resolved.dentry);

out_put_parent:
    path_put(&parent);
out_put_resolved:
    path_put(&resolved);
    return ret;
}

int vfs_rmdir(const char *path) {
    struct path resolved = {0};
    struct path parent = {0};
    struct qstr last = {0};
    int ret;

    CHECK(path != NULL, "fs: invalid rmdir path", return -EINVAL;);

    ret = path_lookup_nofollow(path, &resolved);
    if (ret < 0) {
        return ret;
    }

    if (resolved.dentry == resolved.mnt->mnt_root) {
        ret = -EBUSY;
        goto out_put_resolved;
    }

    if (resolved.dentry->d_inode == NULL) {
        ret = -ENOENT;
        goto out_put_resolved;
    }

    if (!S_ISDIR(resolved.dentry->d_inode->i_mode)) {
        ret = -ENOTDIR;
        goto out_put_resolved;
    }

    ret = path_parentat(path, &parent, &last);
    if (ret < 0) {
        goto out_put_resolved;
    }

    if (parent.dentry == NULL || parent.dentry->d_inode == NULL ||
        parent.dentry->d_inode->i_op == NULL ||
        parent.dentry->d_inode->i_op->rmdir == NULL) {
        ret = -EPERM;
        goto out_put_parent;
    }

    ret = parent.dentry->d_inode->i_op->rmdir(parent.dentry->d_inode, resolved.dentry);

out_put_parent:
    path_put(&parent);
out_put_resolved:
    path_put(&resolved);
    return ret;
}

int vfs_rename(const char *old_path, const char *new_path) {
    struct path old_resolved = {0};
    struct path old_parent = {0};
    struct path new_parent = {0};
    struct qstr old_last = {0};
    struct qstr new_last = {0};
    struct dentry *new_dentry = NULL;
    struct dentry *existing = NULL;
    struct inode *inode;
    int ret;

    CHECK(old_path != NULL && new_path != NULL, "fs: invalid rename path", return -EINVAL;);

    ret = path_lookup_nofollow(old_path, &old_resolved);
    if (ret < 0)
        return ret;

    if (old_resolved.dentry == old_resolved.mnt->mnt_root) {
        ret = -EBUSY;
        goto out_put_old_resolved;
    }

    inode = old_resolved.dentry->d_inode;
    if (inode == NULL) {
        ret = -ENOENT;
        goto out_put_old_resolved;
    }

    ret = path_parentat(old_path, &old_parent, &old_last);
    if (ret < 0)
        goto out_put_old_resolved;

    ret = path_parentat(new_path, &new_parent, &new_last);
    if (ret < 0)
        goto out_put_old_parent;

    if (old_parent.mnt != new_parent.mnt) {
        ret = -EXDEV;
        goto out_put_new_parent;
    }

    if (new_parent.dentry == NULL || new_parent.dentry->d_inode == NULL ||
        new_parent.dentry->d_inode->i_op == NULL ||
        new_parent.dentry->d_inode->i_op->rename == NULL) {
        ret = -EPERM;
        goto out_put_new_parent;
    }

    existing = d_lookup(new_parent.dentry, &new_last);
    if (existing != NULL && existing->d_inode != NULL) {
        ret = -EEXIST;
        goto out_put_new_parent;
    }

    new_dentry = existing ? dget(existing) : d_alloc_qstr(new_parent.dentry, &new_last);
    if (new_dentry == NULL) {
        ret = -ENOMEM;
        goto out_put_new_parent;
    }

    ret = new_parent.dentry->d_inode->i_op->rename(
        old_parent.dentry->d_inode, old_resolved.dentry,
        new_parent.dentry->d_inode, new_dentry);
    if (ret == 0) {
        d_add(new_dentry, inode);
        d_add(old_resolved.dentry, NULL);
    }

    dput(new_dentry);
    goto out_put_new_parent;

out_put_new_parent:
    path_put(&new_parent);
out_put_old_parent:
    path_put(&old_parent);
out_put_old_resolved:
    path_put(&old_resolved);
    return ret;
}

int vfs_access(const char *path, int mode) {
    struct path resolved = {0};
    struct inode *inode;
    u16 perms;
    int ret;

    CHECK(path != NULL, "fs: invalid access path", return -EINVAL;);

    if (mode & ~7) {
        return -EINVAL;
    }

    ret = path_lookup(path, &resolved);
    if (ret < 0) {
        return ret;
    }

    inode = resolved.dentry->d_inode;
    if (inode == NULL) {
        ret = -ENOENT;
        goto out_put;
    }

    if (mode == 0) {
        ret = 0;
        goto out_put;
    }

    perms = inode->i_mode;
    if ((mode & 4) && !(perms & (S_IRUSR | S_IRGRP | S_IROTH))) {
        ret = -EACCES;
        goto out_put;
    }
    if ((mode & 2) && !(perms & (S_IWUSR | S_IWGRP | S_IWOTH))) {
        ret = -EACCES;
        goto out_put;
    }
    if ((mode & 1) && !(perms & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        ret = -EACCES;
        goto out_put;
    }

    ret = 0;

out_put:
    path_put(&resolved);
    return ret;
}

int vfs_getcwd(char *buf, size_t size) {
    struct fs_struct *fs = current->fs;
    struct path root = {0};
    struct path current_path = {0};
    size_t pos;
    size_t out_len;
    size_t i;
    int ret = 0;

    if (buf == NULL || size == 0) {
        return -EINVAL;
    }
    if (fs == NULL) {
        return -ENOENT;
    }

    spin_lock(&fs->lock);
    root = fs->root;
    current_path = fs->pwd;
    path_get(&root);
    path_get(&current_path);
    spin_unlock(&fs->lock);

    if (root.mnt == NULL || root.dentry == NULL ||
        current_path.mnt == NULL || current_path.dentry == NULL) {
        ret = -ENOENT;
        goto out_put_paths;
    }

    if (path_is_equal(&current_path, &root)) {
        if (size < 2) {
            ret = -ERANGE;
            goto out_put_paths;
        }
        buf[0] = '/';
        buf[1] = '\0';
        ret = 2;
        goto out_put_paths;
    }

    pos = size;
    buf[--pos] = '\0';

    while (!path_is_equal(&current_path, &root)) {
        const char *name = NULL;
        size_t name_len = 0;

        if (current_path.dentry == current_path.mnt->mnt_root) {
            if (current_path.mnt->mnt_mountpoint == NULL ||
                current_path.mnt->mnt_mountpoint->d_name.name == NULL) {
                ret = -ENOENT;
                goto out_put_paths;
            }
            name = current_path.mnt->mnt_mountpoint->d_name.name;
            name_len = current_path.mnt->mnt_mountpoint->d_name.len;
        } else {
            if (current_path.dentry->d_name.name == NULL) {
                ret = -ENOENT;
                goto out_put_paths;
            }
            name = current_path.dentry->d_name.name;
            name_len = current_path.dentry->d_name.len;
        }

        if (name_len == 0 || pos < (name_len + 1)) {
            ret = -ERANGE;
            goto out_put_paths;
        }

        pos -= name_len;
        memcpy(buf + pos, name, name_len);
        buf[--pos] = '/';

        ret = path_step_up(&current_path, &root);
        if (ret < 0) {
            goto out_put_paths;
        }
    }

    out_len = size - pos;
    for (i = 0; i < out_len; i++) {
        buf[i] = buf[pos + i];
    }
    ret = (int)out_len;

out_put_paths:
    path_put(&current_path);
    path_put(&root);
    return ret;
}
