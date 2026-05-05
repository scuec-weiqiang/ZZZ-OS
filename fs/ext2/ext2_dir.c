#include <mm/page.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/pagecache.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/bitops.h>
#include "ext2_types.h"

extern int ext2_block_mapping(struct inode *inode, u32 index);
extern int ext2_block_set_mapping(struct inode *inode, u32 index);
extern struct inode *ext2_new_inode(struct inode *dir, u16 mode);
extern int ext2_write_inode(struct inode *inode);
extern u32 ext2_alloc_bno(struct super_block *sb);
extern int ext2_release_bno(struct super_block *sb, u32 bno);
extern int ext2_release_ino(struct super_block *sb, u32 ino);

/* 目录的空闲槽信息 */
typedef struct dir_slot {
    u64 page_index;
    u32 offset;
    u32 prev_offset;
    u32 prev_real_len;
    u32 free_len;
    bool found;
} dir_slot_t;

static unsigned char ext2_filetype_table[EXT2_FT_MAX] = {
	[EXT2_FT_UNKNOWN]	= DT_UNKNOWN,
	[EXT2_FT_REG_FILE]	= DT_REG,
	[EXT2_FT_DIR]		= DT_DIR,
	[EXT2_FT_CHRDEV]	= DT_CHR,
	[EXT2_FT_BLKDEV]	= DT_BLK,
	[EXT2_FT_FIFO]		= DT_FIFO,
	[EXT2_FT_SOCK]		= DT_SOCK,
	[EXT2_FT_SYMLINK]	= DT_LNK,
};

#define S_SHIFT 12
static unsigned char ext2_type_by_mode[S_IFMT >> S_SHIFT] = {
	[S_IFREG >> S_SHIFT]	= EXT2_FT_REG_FILE,
	[S_IFDIR >> S_SHIFT]	= EXT2_FT_DIR,
	[S_IFCHR >> S_SHIFT]	= EXT2_FT_CHRDEV,
	[S_IFBLK >> S_SHIFT]	= EXT2_FT_BLKDEV,
	[S_IFIFO >> S_SHIFT]	= EXT2_FT_FIFO,
	[S_IFSOCK >> S_SHIFT]	= EXT2_FT_SOCK,
	[S_IFLNK >> S_SHIFT]	= EXT2_FT_SYMLINK,
};

// static inline void ext2_set_de_type(struct ext2_dir_entry_2 *de, struct inode *inode) {
// 	u32 mode = inode->i_mode;
// 	de->file_type = ext2_type_by_mode[(mode & S_IFMT)>>S_SHIFT];
// }

struct page *ext2_get_page(struct inode *inode, u32 index) {
    struct page *page = NULL;
    int ret = 0;

    if (inode == NULL) {
        return ERR_PTR(-EINVAL);
    }

    page = pagecache_get_page(inode->i_mapping, index, FGP_CREAT);
    if (IS_ERR(page)) {
        return page;
    }
    if (page == NULL) {
        return ERR_PTR(-ENOENT);
    }

    lock_page(page);
    if (!PageUptodate(page)) {
        if (inode->i_mapping->a_ops == NULL || inode->i_mapping->a_ops->readpage == NULL) {
            unlock_page(page);
            pagecache_put_page(page);
            return ERR_PTR(-EINVAL);
        }

        ret = inode->i_mapping->a_ops->readpage(page);
        if (ret == 0) {
            SetPageUptodate(page);
        }
    }
    unlock_page(page);

    if (ret < 0) {
        pagecache_put_page(page);
        return ERR_PTR(ret);
    }

    return page;
}

void ext2_put_page(struct page *page) {
    pagecache_put_page(page);
}

static int ext2_readpage(struct page *page) {
    struct inode *inode= page->mapping->host;
    struct blkdev *bdev = page->mapping->host->i_sb->s_bdev;
    u8 *page_buf = (u8*)page_address(page);

    u32 block_size = inode->i_sb->s_blocksize;
    u32 blocks_per_page = PAGE_SIZE / block_size;
    u32 first_file_block = page->index * blocks_per_page;

    u32 file_block = 0;
    u32 disk_block = 0;

    int ret = -EIO;

    for (int i = 0; i < blocks_per_page; i++) {
        file_block = first_file_block + i;
        disk_block = ext2_block_mapping(inode, file_block);

        if (disk_block == 0) {
            memset(page_buf + i * block_size, 0, block_size);
            continue;
        }

        u64 pos = (u64)disk_block * block_size;
        ret = blkdev_read(bdev, page_buf + i * block_size, block_size, pos);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

static int ext2_writepage(struct page *page) {
    struct inode *inode = page->mapping->host;
    struct blkdev *bdev = inode->i_sb->s_bdev;
    u8 *page_buf = (u8*)page_address(page);

    u32 block_size = inode->i_sb->s_blocksize;
    u32 blocks_per_page = PAGE_SIZE / block_size;
    u32 first_file_block = page->index * blocks_per_page;

    int ret = -EIO;

    for (int i = 0; i < blocks_per_page; i++) {
        u32 file_block = first_file_block + i;
        int disk_block = ext2_block_mapping(inode, file_block);

        if (disk_block == 0)
            continue;

        u64 pos = (u64)disk_block * block_size;
        ret = blkdev_write(bdev, page_buf + i * block_size, block_size, pos);
        if (ret < 0)
            return ret;
    }
    return 0;
}

static int ext2_commit_dir_page(struct inode *dir, struct page *page) {
    int ret;

    SetPageDirty(page);

    ret = ext2_writepage(page);
    if (ret < 0)
        return ret;

    ext2_put_page(page);

    return ext2_write_inode(dir);
}

const struct address_space_operations ext2_aops = {
    .readpage = ext2_readpage,
    .writepage = ext2_writepage,
};

static u32 last_valid_byte(struct inode *inode, u32 page_nr) {
    u32 last_byte = inode->i_size;
    last_byte -= page_nr * PAGE_SIZE;
    if (last_byte > PAGE_SIZE) {
        last_byte = PAGE_SIZE;
    }

    return last_byte;
}

static struct ext2_dir_entry_2 *ext2_find_entry(struct inode *dir, struct qstr *child, struct page **out_page) {
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;
    u32 rec_len = EXT2_DIR_REC_LEN(child->len);
    struct page *page = NULL;
    char *buf = NULL;
    struct ext2_dir_entry_2 *child_entry = NULL;

    for (int i = 0; i < page_nr; i++) {
        page = ext2_get_page(dir, i);
        if (IS_ERR(page)) {
            return ERR_CAST(page);
        }
        buf = (char *)page_address(page);
        u32 valid_bytes_in_page = last_valid_byte(dir, i);
        char *end = (char*)(valid_bytes_in_page - rec_len);
        u32 offset = 0;
        while (offset < (u32)end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);
            if (entry->inode != 0 && entry->name_len == child->len &&
                memcmp(entry->name, child->name, child->len) == 0) {
                child_entry = entry;
                *out_page = page;
                goto found;
            }
            offset += entry->rec_len;
        }
    }

found:
    return child_entry;
}

int ext2_readdir(struct file *fp, struct dir_context *ctx) {
    struct inode *dir = file_inode(fp);
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;
    struct page *page = NULL;
    char *buf = NULL;
    
    u32 pos = ctx->pos;
    u32 start_page = pos / PAGE_SIZE;
    u32 offset = pos % PAGE_SIZE;

    for (int i = start_page; i < page_nr; i++) {
        page = ext2_get_page(dir, i);
        if (IS_ERR(page)) {
            return PTR_ERR(page);
        }
        buf = (char *)page_address(page);
        u32 valid_end = last_valid_byte(dir, i);
        while (offset < (u32)valid_end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);
            if (entry->rec_len == 0) {
                ext2_put_page(page);
                return -EIO;
            }

            if (entry->inode != 0) {
                unsigned int d_type = ext2_filetype_table[entry->file_type];
                int err = ctx->actor(ctx, entry->name, entry->name_len, offset, entry->inode, d_type);
                if (err < 0) {
                    ext2_put_page(page);
                    return err;
                }
                
            }
            offset += entry->rec_len;
            ctx->pos = i * PAGE_SIZE + offset;
        }
        offset = 0;
        ext2_put_page(page);
    }

    return 0;
}

u32 ext2_find(struct inode *dir, struct qstr *child) {
    struct page *page = NULL;
    struct ext2_dir_entry_2 *entry = ext2_find_entry(dir, child, &page);
    if (IS_ERR(entry)) {
        return 0;
    }
    if (entry == NULL) {
        return 0;
    }
    ext2_put_page(page);
    return entry->inode;
}

const struct file_operations ext2_dir_operations = {
    .iterate = ext2_readdir,
};

static unsigned char ext2_mode_to_ftype(u16 mode) {
    return ext2_type_by_mode[(mode & S_IFMT) >> S_SHIFT];
}

/* 找到一个entry的空槽 */
int ext2_find_slot(struct inode *dir, size_t name_len, dir_slot_t *slot_out) {
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;
    u32 rec_len = EXT2_DIR_REC_LEN(name_len);

    memset(slot_out, 0, sizeof(*slot_out));

    for (u32 i = 0; i < page_nr; i++) {
        struct page *page = ext2_get_page(dir, i);
        if (IS_ERR(page))
            return PTR_ERR(page);

        char *buf = (char *)page_address(page);
        u32 valid_end = last_valid_byte(dir, i);
        u32 offset = 0;

        while (offset + rec_len <= valid_end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);

            if (entry->inode == 0) {
                u32 free_len = entry->rec_len;
                if (free_len < rec_len)
                    break;
                slot_out->page_index = i;
                slot_out->offset = offset;
                slot_out->free_len = free_len;
                slot_out->found = true;
                ext2_put_page(page);
                return 0;
            }

            // 如果某个entry可以拆分 
            u32 real_len = EXT2_DIR_REC_LEN(entry->name_len);
            if (entry->rec_len >= real_len + rec_len) {
                slot_out->page_index = i;
                slot_out->offset = offset + real_len;
                slot_out->prev_offset = offset;
                slot_out->prev_real_len = real_len;
                slot_out->free_len = entry->rec_len - real_len;
                slot_out->found = true;
                ext2_put_page(page);
                return 0;
            }

            offset += entry->rec_len;
        }

        ext2_put_page(page);
    }

    return -ENOMEM;
}

static void ext2_init_dir_entry(struct ext2_dir_entry_2 *de, u32 ino, u16 mode, const char *name, u32 name_len, u16 rec_len) {
    de->inode = ino;
    de->rec_len = rec_len;
    de->name_len = name_len;
    de->file_type = ext2_mode_to_ftype(mode);
    memcpy(de->name, name, name_len);
}

int ext2_init_dot_entries(struct inode *new_dir, u32 parent_ino) {
    struct page *page;
    u8 *buf;
    u32 block_size;
    u32 dot_rec_len;
    struct ext2_dir_entry_2 *dot;
    struct ext2_dir_entry_2 *dotdot;
    int ret;

    ret = ext2_block_set_mapping(new_dir, 0);
    if (ret < 0)
        return ret;

    block_size = new_dir->i_sb->s_blocksize;
    dot_rec_len = EXT2_DIR_REC_LEN(1);

    page = ext2_get_page(new_dir, 0);
    if (IS_ERR(page))
        return PTR_ERR(page);

    buf = page_address(page);
    memset(buf, 0, block_size);

    dot = (struct ext2_dir_entry_2 *)buf;
    ext2_init_dir_entry(dot,new_dir->i_ino,EXT2_FT_DIR,".",1,dot_rec_len);

    dotdot = (struct ext2_dir_entry_2 *)(buf + dot_rec_len);
    ext2_init_dir_entry(dotdot,parent_ino,EXT2_FT_DIR,"..",2,block_size - dot_rec_len);
    SetPageDirty(page);
    ret = pagecache_write_page(page); // 立即写回
    if (ret < 0) {
        ext2_put_page(page);
        return ret;
    }
    
    ext2_put_page(page);
    return ret;
}

static int ext2_add_entry_to_slot(struct inode *parent, const char *name, u32 name_len, 
                                                u32 ino, u16 mode, const dir_slot_t *slot) {
    struct page *page;
    char *buf;
    struct ext2_dir_entry_2 *new_entry;

    page = ext2_get_page(parent, slot->page_index);
    if (IS_ERR(page))
        return PTR_ERR(page);

    buf = page_address(page);

    if (slot->prev_offset != 0 || slot->prev_real_len != 0) {
        struct ext2_dir_entry_2 *prev;
        prev = (struct ext2_dir_entry_2 *)(buf + slot->prev_offset);
        prev->rec_len = slot->prev_real_len;
    }

    new_entry = (struct ext2_dir_entry_2 *)(buf + slot->offset);
    ext2_init_dir_entry(new_entry, ino, mode, name, name_len, slot->free_len);
    SetPageDirty(page);
    pagecache_write_page(page); // 立即写回

    ext2_put_page(page);

    return ext2_write_inode(parent);
}

static int ext2_add_entry_new_block(struct inode *parent, const char *name, u32 name_len, u32 ino, u16 mode) {
    u32 block_size = parent->i_sb->s_blocksize;
    u32 file_block;
    u32 page_index;
    u32 page_offset;
    u32 new_end;
    struct page *page;
    u8 *page_buf;
    struct ext2_dir_entry_2 *de;
    int ret;

    file_block = (parent->i_size + block_size - 1) / block_size;

    ret = ext2_block_set_mapping(parent, file_block);
    if (ret < 0)
        return ret;

    page_index = (file_block * block_size) / PAGE_SIZE;
    page_offset = (file_block * block_size) % PAGE_SIZE;

    page = ext2_get_page(parent, page_index);
    if (IS_ERR(page))
        return PTR_ERR(page);

    page_buf = page_address(page);
    memset(page_buf + page_offset, 0, block_size);
    de = (struct ext2_dir_entry_2 *)(page_buf + page_offset);
    ext2_init_dir_entry(de, ino, mode, name, name_len, block_size);


    SetPageDirty(page);
    // 简化处理，立即写回
    pagecache_write_page(page);
    ext2_put_page(page);

    new_end = (file_block + 1) * block_size;
    if (new_end > parent->i_size)
        parent->i_size = new_end;

    return ext2_write_inode(parent);
}

static int ext2_add_entry(struct inode *parent, const char *name, u32 name_len, u32 ino, u16 mode) {
    int ret;
    dir_slot_t slot;

    ret = ext2_find_slot(parent, name_len, &slot);
    if (ret < 0)
        return ret;

    if (slot.found) {
        return ext2_add_entry_to_slot(parent, name, name_len, ino, mode, &slot);
    }
        
    return ext2_add_entry_new_block(parent, name, name_len, ino, mode);
}

static int ext2_create(struct inode *dir, struct dentry *dentry, u16 mode) {
    struct inode *new_inode;

    if (dir == NULL || dentry == NULL)
        return -EINVAL;

    new_inode = ext2_new_inode(dir, S_IFREG | mode);
    if (IS_ERR(new_inode))
        return PTR_ERR(new_inode);

    d_add(dentry, new_inode);

    
    int ret = ext2_add_entry(dir, dentry->d_name.name, dentry->d_name.len,
                             new_inode->i_ino, S_IFREG);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, new_inode->i_ino);
        return ret;
    }

    dir->i_mtime = dir->i_ctime;
    ext2_write_inode(dir);

    return 0;
}

static int ext2_mkdir(struct inode *dir, struct dentry *dentry, u16 mode) {
    struct inode *new_dir;
    int ret;

    if (dir == NULL || dentry == NULL)
        return -EINVAL;

    new_dir = ext2_new_inode(dir, S_IFDIR | mode);
    if (IS_ERR(new_dir))
        return PTR_ERR(new_dir);

    ret = ext2_init_dot_entries(new_dir, dir->i_ino);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, new_dir->i_ino);
        return ret;
    }

    d_add(dentry, new_dir);

    ret = ext2_add_entry(dir, dentry->d_name.name, dentry->d_name.len,
                         new_dir->i_ino, S_IFDIR);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, new_dir->i_ino);
        dentry->d_inode = NULL;
        return ret;
    }

    dir->i_nlink++;
    dir->i_mtime = dir->i_ctime;
    ext2_write_inode(dir);

    return 0;
}

static int ext2_mknod(struct inode *dir, struct dentry *dentry, u16 mode, dev_t dev) {
    struct inode *new_inode;

    if (dir == NULL || dentry == NULL)
        return -EINVAL;

    new_inode = ext2_new_inode(dir, mode);
    if (IS_ERR(new_inode))
        return PTR_ERR(new_inode);

    if (S_ISBLK(mode) || S_ISCHR(mode))
        new_inode->i_rdev = dev;

    d_add(dentry, new_inode);

    int ret = ext2_add_entry(dir, dentry->d_name.name, dentry->d_name.len,
                             new_inode->i_ino, mode);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, new_inode->i_ino);
        dentry->d_inode = NULL;
        return ret;
    }

    dir->i_mtime = dir->i_ctime;
    ext2_write_inode(dir);

    return 0;
}

const struct inode_operations ext2_dir_inode_operations = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .mkdir = ext2_mkdir,
    .mknod = ext2_mknod,
};
