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
    
    if (vfs_search_mount(next, &mounted) == 0) {
        new_path.mnt = mntget(mounted);
        new_path.dentry = dget(mounted->mnt_root);
        dput(next);
    } else {
        new_path.mnt = mntget(path->mnt);
        new_path.dentry = next;
    }
    
    path_put(path);
    
    *path = new_path;
    return 0;
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
    struct path root = {0};
    struct path current_path = {0};
    struct qstr name;
    int ret;

    CHECK(path != NULL, "fs: invalid path", return -EINVAL;);
    CHECK(out != NULL, "fs: invalid lookup output", return -EINVAL;);
    CHECK(path[0] != '\0', "fs: empty path", return -EINVAL;);

    ret = path_init_start(path, &root);
    if (ret < 0) {
        return ret;
    }

    ret = path_init_start(path, &current_path);
    if (ret < 0) {
        path_put(&root);
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
            ret = path_step_up(&current_path, &root);
        } else {
            dprintk("lookup component '%s' under dentry '%s'\n", name.name, current_path.dentry->d_name.name);
            ret = path_step_down(&current_path, &name);
        }
        if (ret < 0) {
            goto err_out;
        }
    }

    *out = current_path;
    path_put(&root);
    return 0;

err_out:
    path_put(&current_path);
    path_put(&root);
    out->mnt = NULL;
    out->dentry = NULL;
    return ret;
}

int path_parentat(const char *path, struct path *parent, struct qstr *last) {
    struct path_cursor cur;
    struct path root = {0};
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

    ret = path_init_start(path, &root);
    if (ret < 0) {
        return ret;
    }

    ret = path_init_start(path, &current_path);
    if (ret < 0) {
        path_put(&root);
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
            path_put(&root);
            return 0;
        }

        save = cur;
        next_ret = next_component(&save, &next_name);
        if (next_ret > 0) {
            *parent = current_path;
            *last = name;
            path_put(&root);
            return 0;
        }

        if (qstr_is_dot(&name)) {
            continue;
        }

        if (qstr_is_dotdot(&name)) {
            ret = path_step_up(&current_path, &root);
        } else {
            ret = path_step_down(&current_path, &name);
        }
        if (ret < 0) {
            goto err_out;
        }
    }

err_out:
    path_put(&current_path);
    path_put(&root);
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

struct dentry* vfs_mkdir(const char* path,u16 mode) {
    CHECK(path != NULL, "", return NULL;);
    CHECK(mode > 0, "", return NULL;);

    struct path resolved_parent;
    struct qstr child_name;

    int ret = path_parentat(path, &resolved_parent, &child_name);

    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    struct dentry *child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    dprintk("get parent dentry %s, parent node = %xu\n", resolved_parent.dentry->d_name.name, resolved_parent.dentry->d_inode->i_private);

    ret = resolved_parent.dentry->d_inode->i_op->mkdir(resolved_parent.dentry->d_inode, child_dentry, mode);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }
    return child_dentry;
}

struct dentry *vfs_create(const char *path, u16 mode) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    CHECK(path != NULL, "", return NULL;);

    ret = path_parentat(path, &resolved_parent, &child_name);
    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
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

    child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    ret = resolved_parent.dentry->d_inode->i_op->mknod(resolved_parent.dentry->d_inode, child_dentry, mode, dev);
    path_put(&resolved_parent);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
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
