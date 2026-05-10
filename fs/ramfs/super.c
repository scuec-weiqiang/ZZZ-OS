#include <fs/dcache.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fs_context.h>
#include <fs/inode.h>
#include <fs/namei.h>
#include <fs/fs.h>
#include <fs/ramfs.h>
#include <os/init.h>
#include <fs/super.h>
#include <os/check.h>
#include <os/container_of.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/err.h>

#define RAMFS_MAGIC 0x52414d32U

struct ramfs_node;

struct ramfs_dentry {
    char *name;
    struct ramfs_node *node;
    struct list_head sibling;
};

struct ramfs_node {
    ino_t ino;
    u16 mode;
    u32 nlink;
    u32 size;
    dev_t rdev;
    timespec_t atime;
    timespec_t mtime;
    timespec_t ctime;
    struct ramfs_node *parent;
    struct list_head sb_list;
    union {
        struct {
            struct list_head children;
        } dir;
        struct {
            char *data;
            u32 capacity;
        } reg;
    } u;
};

struct ramfs_sb_info {
    ino_t next_ino;
    struct list_head nodes;
};

#define S_SHIFT 12
static unsigned char ramfs_type_by_mode[S_IFMT >> S_SHIFT] = {
	[S_IFREG >> S_SHIFT]	= DT_REG,
	[S_IFDIR >> S_SHIFT]	= DT_DIR,
	[S_IFCHR >> S_SHIFT]	= DT_CHR,
	[S_IFBLK >> S_SHIFT]	= DT_BLK,
	[S_IFIFO >> S_SHIFT]	= DT_FIFO,
	[S_IFSOCK >> S_SHIFT]	= DT_SOCK,
	[S_IFLNK >> S_SHIFT]	= DT_LNK,
};

static struct ramfs_sb_info *RAMFS_SB(struct super_block *sb)
{
    return (struct ramfs_sb_info *)sb->s_fs_info;
}

static struct ramfs_node *RAMFS_NODE(struct inode *inode)
{
    return (struct ramfs_node *)inode->i_private;
}

static struct ramfs_node *ramfs_find_node_by_ino(struct super_block *sb, ino_t ino) {
    struct ramfs_sb_info *sbi = RAMFS_SB(sb);
    struct list_head *pos = NULL;

    if (sbi == NULL) {
        return NULL;
    }

    list_for_each(pos, &sbi->nodes) {
        struct ramfs_node *node = container_of(pos, struct ramfs_node, sb_list);
        if (node->ino == ino) {
            return node;
        }
    }

    return NULL;
}

static struct ramfs_dentry *ramfs_find_child(struct ramfs_node *dir, const char *name, u16 len) {
    struct list_head *pos = NULL;

    if (dir == NULL || !S_ISDIR(dir->mode)) {
        return NULL;
    }

    list_for_each(pos, &dir->u.dir.children) {
        struct ramfs_dentry *entry = container_of(pos, struct ramfs_dentry, sibling);
        if (strlen(entry->name) == len && memcmp(entry->name, name, len) == 0) {
            return entry;
        }
    }

    return NULL;
}

static int ramfs_qstr_is_dot(const struct qstr *name)
{
    return name != NULL && name->len == 1 && name->name[0] == '.';
}

static int ramfs_qstr_is_dotdot(const struct qstr *name)
{
    return name != NULL && name->len == 2 &&
           name->name[0] == '.' && name->name[1] == '.';
}

static int ramfs_inode_refresh(struct inode *inode) {
    struct ramfs_node *node = RAMFS_NODE(inode);

    CHECK(inode != NULL, "fs: invalid ramfs inode", return -1;);
    CHECK(node != NULL, "fs: invalid ramfs private node", return -1;);

    inode->i_ino = node->ino;
    inode->i_mode = node->mode;
    inode->i_nlink = node->nlink;
    inode->i_size = node->size;
    inode->i_rdev = node->rdev;
    inode->i_atime = node->atime;
    inode->i_mtime = node->mtime;
    inode->i_ctime = node->ctime;

    return 0;
}

static struct inode *ramfs_iget(struct super_block *sb, ino_t ino);

static struct ramfs_node *ramfs_node_create(struct super_block *sb, struct ramfs_node *parent, u16 mode, dev_t rdev) {
    struct ramfs_sb_info *sbi = RAMFS_SB(sb);
    struct ramfs_node *node = NULL;

    CHECK(sbi != NULL, "fs: invalid ramfs sb info", return NULL;);

    node = kmalloc(sizeof(*node));
    CHECK(node != NULL, "fs: alloc ramfs node failed", return NULL;);
    memset(node, 0, sizeof(*node));

    node->ino = sbi->next_ino++;
    node->mode = mode;
    node->nlink = S_ISDIR(mode) ? 2 : 1;
    node->rdev = rdev;
    node->parent = parent;
    INIT_LIST_HEAD(&node->sb_list);

    if (S_ISDIR(mode)) {
        INIT_LIST_HEAD(&node->u.dir.children);
    }

    list_add_tail(&sbi->nodes, &node->sb_list);
    return node;
}

static int ramfs_dir_add_child(struct ramfs_node *dir, const char *name, struct ramfs_node *child) {
    struct ramfs_dentry *entry = NULL;
    size_t len = 0;

    CHECK(dir != NULL && S_ISDIR(dir->mode), "fs: invalid ramfs dir", return -1;);
    CHECK(name != NULL, "fs: invalid ramfs child name", return -1;);
    CHECK(child != NULL, "fs: invalid ramfs child node", return -1;);
    CHECK(ramfs_find_child(dir, name, strlen(name)) == NULL, "fs: ramfs child already exists", return -1;);

    entry = kmalloc(sizeof(*entry));
    CHECK(entry != NULL, "fs: alloc ramfs dir entry failed", return -1;);
    memset(entry, 0, sizeof(*entry));

    len = strlen(name);
    entry->name = kmalloc(len + 1);
    CHECK(entry->name != NULL, "fs: alloc ramfs dir entry name failed", kfree(entry); return -1;);
    memcpy(entry->name, name, len);
    entry->name[len] = '\0';
    entry->node = child;
    INIT_LIST_HEAD(&entry->sibling);
    list_add_tail(&dir->u.dir.children, &entry->sibling);

    if (S_ISDIR(child->mode)) {
        dir->nlink++;
    }

    return 0;
}

static struct inode *ramfs_new_inode(struct inode *dir, u16 mode, struct qstr *name) {
    struct super_block *sb = dir->i_sb;
    struct ramfs_node *dir_node = RAMFS_NODE(dir);
    struct ramfs_node *new_node = NULL;

    CHECK(sb != NULL, "fs: invalid ramfs new inode sb", return NULL;);
    CHECK(dir_node != NULL && S_ISDIR(dir_node->mode), "fs: invalid ramfs new inode dir", return NULL;);
    CHECK(name != NULL, "fs: invalid ramfs new inode name", return NULL;);
    CHECK(ramfs_find_child(dir_node, name->name, name->len) == NULL, "fs: ramfs entry exists", return NULL;);
     
    new_node = ramfs_node_create(sb, dir_node, mode, 0);
    CHECK(new_node != NULL, "fs: create ramfs node failed", return NULL;);

    CHECK(ramfs_dir_add_child(dir_node, name->name, new_node) == 0, "fs: add ramfs child failed", return NULL;);
    return ramfs_iget(sb, new_node->ino);
}

static struct inode *ramfs_alloc_inode(struct super_block *sb) {
    struct inode *inode = kmalloc(sizeof(*inode));
    CHECK(inode != NULL, "fs: alloc ramfs inode failed", return NULL;);
    memset(inode, 0, sizeof(*inode));
    inode->i_sb = sb;
    return inode;
}

static void ramfs_destroy_inode(struct inode *inode) {
    kfree(inode);
}

static int ramfs_write_inode(struct inode *inode) {
    struct ramfs_node *node = RAMFS_NODE(inode);

    CHECK(inode != NULL, "fs: invalid ramfs inode", return -1;);
    CHECK(node != NULL, "fs: invalid ramfs private node", return -1;);

    node->mode = inode->i_mode;
    node->nlink = inode->i_nlink;
    node->size = inode->i_size;
    node->rdev = inode->i_rdev;
    node->atime = inode->i_atime;
    node->mtime = inode->i_mtime;
    node->ctime = inode->i_ctime;
    return 0;
}

static int ramfs_sync_fs(struct super_block *sb) {
    (void)sb;
    return 0;
}

static struct super_operations ramfs_super_ops = {
    .put_super = NULL,
    .write_inode = ramfs_write_inode,
    .sync_fs = ramfs_sync_fs,
    .alloc_inode = ramfs_alloc_inode,
    .destroy_inode = ramfs_destroy_inode,
};

static int ramfs_file_open(struct inode *inode, struct file *file) {
    (void)inode;
    (void)file;
    return 0;
}

static ssize_t ramfs_file_read(struct file *file, char *buf, size_t len, loff_t *ppos) {
    struct ramfs_node *node = RAMFS_NODE(file->f_inode);
    size_t avail = 0;

    CHECK(node != NULL && S_ISREG(node->mode), "fs: invalid ramfs file read", return -1;);
    CHECK(buf != NULL && ppos != NULL, "fs: invalid ramfs file read args", return -1;);

    if ((u32)(*ppos) >= node->size) {
        return 0;
    }

    avail = node->size - (u32)(*ppos);
    if (len > avail) {
        len = avail;
    }

    if (len > 0 && node->u.reg.data != NULL) {
        memcpy(buf, node->u.reg.data + *ppos, len);
    }
    *ppos += len;
    return (ssize_t)len;
}

static ssize_t ramfs_file_write(struct file *file, const char *buf, size_t len, loff_t *ppos) {
    struct ramfs_node *node = RAMFS_NODE(file->f_inode);
    u32 end = 0;
    u32 new_cap = 0;
    char *new_data = NULL;

    CHECK(node != NULL && S_ISREG(node->mode), "fs: invalid ramfs file write", return -1;);
    CHECK(buf != NULL && ppos != NULL, "fs: invalid ramfs file write args", return -1;);

    end = (u32)(*ppos) + (u32)len;
    if (end > node->u.reg.capacity) {
        new_cap = node->u.reg.capacity ? node->u.reg.capacity : 64;
        while (new_cap < end) {
            new_cap <<= 1;
        }
        new_data = kmalloc(new_cap);
        CHECK(new_data != NULL, "fs: grow ramfs file failed", return -1;);
        memset(new_data, 0, new_cap);
        if (node->u.reg.data != NULL && node->size > 0) {
            memcpy(new_data, node->u.reg.data, node->size);
            kfree(node->u.reg.data);
        }
        node->u.reg.data = new_data;
        node->u.reg.capacity = new_cap;
    }

    memcpy(node->u.reg.data + *ppos, buf, len);
    *ppos += len;
    if (end > node->size) {
        node->size = end;
    }

    ramfs_inode_refresh(file->f_inode);
    return (ssize_t)len;
}

static int ramfs_readdir (struct file *fp, struct dir_context *ctx) {
    struct ramfs_node *node = RAMFS_NODE(fp->f_inode);
    struct list_head *pos = NULL;
    loff_t entry_pos = 2;
    int ret;

    CHECK(node != NULL && S_ISDIR(node->mode), "fs: invalid ramfs readdir", return -ENOTDIR;);

    if (ctx->pos == 0) {
        ret = ctx->actor(ctx, ".", 1, 1, node->ino, DT_DIR);
        if (ret < 0) {
            return ret;
        }
        ctx->pos = 1;
    }

    if (ctx->pos == 1) {
        ino_t parent_ino = node->parent ? node->parent->ino : node->ino;

        ret = ctx->actor(ctx, "..", 2, 2, parent_ino, DT_DIR);
        if (ret < 0) {
            return ret;
        }
        ctx->pos = 2;
    }

    list_for_each(pos, &node->u.dir.children) {
        struct ramfs_dentry *entry = container_of(pos, struct ramfs_dentry, sibling);
        unsigned int d_type = ramfs_type_by_mode[(entry->node->mode & S_IFMT) >> S_SHIFT];
        if (ctx->pos > entry_pos) {
            entry_pos++;
            continue;
        }

        ret = ctx->actor(ctx, entry->name, strlen(entry->name), entry_pos + 1, entry->node->ino, d_type);
        if (ret < 0) {
            return ret;
        }
        entry_pos++;
        ctx->pos = entry_pos;
    }
    return 0;
}


static struct file_operations ramfs_file_ops = {
    .open = ramfs_file_open,
    .read = ramfs_file_read,
    .write = ramfs_file_write,
};

static struct file_operations ramfs_dir_ops = {
    .open = ramfs_file_open,
    .iterate = ramfs_readdir,
};

struct inode_operations ramfs_dir_iops;
struct inode *ramfs_iget(struct super_block *sb, ino_t ino) {
    struct inode *inode;
    struct ramfs_node *node;

    inode = iget(sb, ino);
    if (inode == NULL)
        return NULL;

    if (inode->i_state != I_NEW)
        return inode;

    node = ramfs_find_node_by_ino(sb, ino);
    if (node == NULL) {
        iput(inode);
        return NULL;
    }

    inode->i_private = node;
    inode->i_mode = node->mode;
    inode->i_size = node->size;
    inode->i_nlink = node->nlink;
    inode->i_rdev = node->rdev;

    if (S_ISDIR(node->mode))
        inode->i_op = &ramfs_dir_iops;
    if (S_ISDIR(node->mode))
        inode->i_fop = &ramfs_dir_ops;
    else if (S_ISREG(node->mode))
        inode->i_fop = &ramfs_file_ops;

    // unlock_new_inode(inode);
    return inode;
}


static struct dentry *ramfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags) {
    struct ramfs_node *dir_node = RAMFS_NODE(dir);
    struct ramfs_dentry *entry = NULL;
    struct inode *inode = NULL;
    ino_t ino = 0;

    (void)flags;
    CHECK(dir_node != NULL && S_ISDIR(dir_node->mode), "fs: invalid ramfs lookup dir", return NULL;);

    if (ramfs_qstr_is_dot(&dentry->d_name)) {
        ino = dir_node->ino;
    } else if (ramfs_qstr_is_dotdot(&dentry->d_name)) {
        ino = dir_node->parent ? dir_node->parent->ino : dir_node->ino;
    } else {
        entry = ramfs_find_child(dir_node, dentry->d_name.name, dentry->d_name.len);
        if (entry == NULL || entry->node == NULL) {
            return NULL;
        }
        ino = entry->node->ino;
    }

    inode = ramfs_iget(dir->i_sb, ino);
    CHECK(inode != NULL, "fs: ramfs iget failed", return NULL;);
    
    d_add(dentry, inode);

    return dentry;
}

static int ramfs_do_create(struct inode *dir, struct dentry *dentry, u16 mode, dev_t dev) {
    struct ramfs_node *dir_node = RAMFS_NODE(dir);

    struct inode *new_inode = NULL;

    CHECK(dir_node != NULL && S_ISDIR(dir_node->mode), "fs: invalid ramfs create dir", return -1;);
    CHECK(ramfs_find_child(dir_node, dentry->d_name.name, dentry->d_name.len) == NULL, "fs: ramfs entry exists", return -1;);

    new_inode = ramfs_new_inode(dir, mode, &dentry->d_name);
    new_inode->i_rdev = dev;
    d_add(dentry, new_inode);
    
    ramfs_inode_refresh(dir);
    return 0;
}

static int ramfs_create(struct inode *dir, struct dentry *dentry, u16 mode)
{
    return ramfs_do_create(dir, dentry, S_IFREG | mode, 0);
}

static int ramfs_mkdir(struct inode *dir, struct dentry *dentry, u16 mode)
{
    return ramfs_do_create(dir, dentry, S_IFDIR | mode, 0);
}

static int ramfs_mknod(struct inode *dir, struct dentry *dentry, u16 mode, dev_t dev)
{
    return ramfs_do_create(dir, dentry, mode, dev);
}

struct inode_operations ramfs_dir_iops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
    .mknod = ramfs_mknod,
};

static int ramfs_init_fs_context(struct fs_context *fc) {
    CHECK(fc != NULL, "fs: invalid ramfs fs_context", return -1;);
    return 0;
}

static int ramfs_get_tree(struct fs_context *fc) {
    struct super_block *sb = NULL;
    struct ramfs_sb_info *sbi = NULL;
    struct ramfs_node *root_node = NULL;
    struct inode *root_inode = NULL;
    struct dentry *root_dentry = NULL;

    CHECK(fc != NULL, "fs: invalid ramfs fs_context", return -1;);

    sb = alloc_super(fc->fs_type);
    CHECK(sb != NULL, "fs: alloc ramfs super failed", return -1;);

    sbi = kmalloc(sizeof(*sbi));
    CHECK(sbi != NULL, "fs: alloc ramfs sb info failed", destroy_super(sb); return -1;);
    memset(sbi, 0, sizeof(*sbi));
    sbi->next_ino = 1;
    INIT_LIST_HEAD(&sbi->nodes);

    sb->s_magic = RAMFS_MAGIC;
    sb->s_blocksize = 4096;
    sb->s_op = &ramfs_super_ops;
    sb->s_fs_info = sbi;

    root_node = ramfs_node_create(sb, NULL, S_IFDIR | 0755, 0);
    CHECK(root_node != NULL, "fs: alloc ramfs root node failed", kfree(sbi); destroy_super(sb); return -1;);
    root_node->parent = root_node;

    root_inode = ramfs_iget(sb, root_node->ino);
    CHECK(root_inode != NULL, "fs: alloc ramfs root inode failed", destroy_super(sb); return -1;);

    root_dentry = d_make_root(root_inode);
    CHECK(root_dentry != NULL, "fs: alloc ramfs root dentry failed",
          iput(root_inode);
          destroy_super(sb);
          return -1;);

    iput(root_inode);
    sb->s_root = root_dentry;
    fc->root = root_dentry;
    return 0;
}

static void ramfs_kill_sb(struct super_block *sb) {
    struct ramfs_sb_info *sbi = RAMFS_SB(sb);
    struct list_head *pos = NULL;
    struct list_head *n = NULL;

    if (sbi != NULL) {
        list_for_each_safe(pos, n, &sbi->nodes) {
            struct ramfs_node *node = container_of(pos, struct ramfs_node, sb_list);
            list_del(&node->sb_list);

            if (S_ISDIR(node->mode)) {
                struct list_head *p = NULL;
                struct list_head *pn = NULL;
                list_for_each_safe(p, pn, &node->u.dir.children) {
                    struct ramfs_dentry *entry = container_of(p, struct ramfs_dentry, sibling);
                    list_del(&entry->sibling);
                    kfree(entry->name);
                    kfree(entry);
                }
            } else if (S_ISREG(node->mode) && node->u.reg.data != NULL) {
                kfree(node->u.reg.data);
            }
            kfree(node);
        }
        kfree(sbi);
    }
    destroy_super(sb);
}

struct file_system_type ramfs_fs_type = {
    .name = "ramfs",
    .init_fs_context = ramfs_init_fs_context,
    .get_tree = ramfs_get_tree,
    .kill_sb = ramfs_kill_sb,
};

static int ramfs_register_filesystem_init(void)
{
    return register_filesystem(&ramfs_fs_type);
}

fs_initcall(ramfs_register_filesystem_init);

int ramfs_debug_ls(const char *path) {
    struct dentry *dentry = NULL;
    struct ramfs_node *node = NULL;
    struct list_head *pos = NULL;

    CHECK(path != NULL, "fs: invalid ls path", return -1;);

    dentry = vfs_lookup(path);
    CHECK(dentry != NULL, "fs: ls lookup failed", return -1;);
    CHECK(dentry->d_inode != NULL, "fs: ls negative dentry", dput(dentry); return -1;);

    node = RAMFS_NODE(dentry->d_inode);
    CHECK(node != NULL && S_ISDIR(node->mode), "fs: ls target is not directory", dput(dentry); return -1;);

    list_for_each(pos, &node->u.dir.children) {
        struct ramfs_dentry *entry = container_of(pos, struct ramfs_dentry, sibling);
        printk("%s  ", entry->name);
    }
    // printk("\n");

    dput(dentry);
    return 0;
}
