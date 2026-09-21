#include <mm/page.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/pagecache.h>
#include <fs/blkdev.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/bitops.h>
#include <os/check.h>
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

static int ext2_submit_page_batch(struct blkdev *bdev, u8 *page_buf,
                                  u32 block_size, u32 page_block,
                                  u32 disk_block, u32 nr_blocks,
                                  bool write)
{
    size_t len;
    u64 pos;

    if (nr_blocks == 0)
        return 0;

    len = (size_t)nr_blocks * block_size;
    pos = (u64)disk_block * block_size;

    if (write)
        return blkdev_write(bdev, page_buf + page_block * block_size,
                            len, pos);

    return blkdev_read(bdev, page_buf + page_block * block_size,
                       len, pos);
}

static int ext2_rwpage(struct page *page, bool write)
{
    struct inode *inode;
    struct blkdev *bdev;
    u8 *page_buf;
    u32 block_size;
    u32 blocks_per_page;
    u32 first_file_block;
    u32 batch_page_block = 0;
    u32 batch_disk_block = 0;
    u32 batch_blocks = 0;
    int ret;

    if (!page || !page->mapping || !page->mapping->host)
        return -EINVAL;

    inode = page->mapping->host;
    bdev = inode->i_sb->s_bdev;
    page_buf = page_address(page);
    block_size = inode->i_sb->s_blocksize;

    if (!bdev || block_size < 512 || block_size > PAGE_SIZE ||
        PAGE_SIZE % block_size != 0 || PAGE_SIZE / block_size > 8)
        return -EINVAL;

    blocks_per_page = PAGE_SIZE / block_size;
    first_file_block = page->index * blocks_per_page;

    if (!write)
        memset(page_buf, 0, PAGE_SIZE);

    for (u32 i = 0; i < blocks_per_page; i++) {
        int disk_block;

        if (write && page->dirty_blocks &&
            !(page->dirty_blocks & (1U << i))) {
            ret = ext2_submit_page_batch(bdev, page_buf, block_size,
                                        batch_page_block, batch_disk_block,
                                        batch_blocks, write);
            if (ret < 0)
                return ret;
            batch_blocks = 0;
            continue;
        }

        disk_block = ext2_block_mapping(inode, first_file_block + i);

        if (disk_block < 0)
            return disk_block;
        if (write && page->dirty_blocks && disk_block == 0)
            return -EIO;

        if (disk_block != 0 && batch_blocks != 0 &&
            (u32)disk_block == batch_disk_block + batch_blocks) {
            batch_blocks++;
            continue;
        }

        ret = ext2_submit_page_batch(bdev, page_buf, block_size, 
            batch_page_block, batch_disk_block, batch_blocks, write);
        if (ret < 0)
            return ret;

        batch_blocks = 0;
        if (disk_block != 0) {
            batch_page_block = i;
            batch_disk_block = (u32)disk_block;
            batch_blocks = 1;
        }
    }

    return ext2_submit_page_batch(bdev, page_buf, block_size,
                                  batch_page_block, batch_disk_block,
                                  batch_blocks, write);
}

static int ext2_read_batch(struct bio *bio)
{
    int ret;

    if (!bio->bi_size)
        return 0;
    ret = submit_bio_wait(bio);
    bio->bi_size = 0;
    bio->bi_vcnt = 0;
    return ret;
}

// 上层持有页面引用并保证读入期间不被修改，页状态由上层更新
// 上层提供一些缓存页，ext2找出缓存页对应的磁盘块，将能连续读取的部分合并成一个批次提交给块设备
int ext2_readpages(struct page **pages, u32 nr_pages)
{
    struct inode *inode;
    struct address_space *mapping;
    struct blkdev *bdev;
    struct request_queue *queue;
    struct bio *bio;
    u32 block_size;
    u32 max_bytes;
    u32 nr_vecs;
    u64 file_size;
    int ret = 0;

    if (!nr_pages) {
        return 0;
    }

    ASSERT(pages != NULL, "ext2: missing page array");
    ASSERT(pages[0] != NULL, "ext2: missing first page");
    ASSERT(pages[0]->mapping != NULL, "ext2: page has no mapping");

    mapping = pages[0]->mapping;
    inode = mapping->host;
    ASSERT(inode != NULL, "ext2: mapping has no inode");
    ASSERT(inode->i_sb != NULL, "ext2: inode has no superblock");
  
    block_size = inode->i_sb->s_blocksize;
    if (!block_size || block_size > PAGE_SIZE || PAGE_SIZE % block_size || block_size % SECTOR_SIZE)
        return -EINVAL;

    bdev = inode->i_sb->s_bdev;
    if (!bdev || !bdev->bd_disk || !bdev->bd_disk->queue)
        return -ENODEV;
    queue = bdev->bd_disk->queue;
    if (!queue->logical_block_size || block_size % queue->logical_block_size)
        return -EINVAL;
    

    // 计算一次 BIO 最大可提交的字节数，确保 BIO 不会超过块设备队列的限制
    max_bytes = queue->max_hw_sectors * SECTOR_SIZE;
    if (max_bytes < block_size) {
        return -EINVAL;
    }

    file_size = inode->i_size;

    // 检查页数组的有效性；EOF 之后的页允许传入，内容填零
    for (u32 i = 0; i < nr_pages; i++) {
        struct page *page = pages[i];

        ASSERT(page != NULL, "ext2: missing page");
        ASSERT(page->mapping == mapping, "ext2: pages have different mappings");
        ASSERT(!PageDirty(page), "ext2: cannot overwrite dirty page");
        ASSERT(!PageUptodate(page), "ext2: page is already uptodate");
        ASSERT(!PageWriteback(page), "ext2: page is under writeback");
        ASSERT(!i || page->index > pages[i - 1]->index, "ext2: page indexes must be strictly increasing");
        if ((u64)page->index > UINT32_MAX / (PAGE_SIZE / block_size))
            return -EFBIG;
    }

    // 限制临时 BIO 大小,页数更多时分批处理，不依赖预读窗口
    nr_vecs = nr_pages < 16 ? nr_pages : 16;
    bio = bio_alloc(nr_vecs);
    if (IS_ERR(bio))
        return PTR_ERR(bio);
    bio->op = REQ_OP_READ;
    bio->bi_bdev = bdev;

    for (u32 i = 0; i < nr_pages; i++) {
        struct page *page = pages[i];
        u64 page_pos = (u64)page->index * PAGE_SIZE;

        memset(pagecache_data(page), 0, PAGE_SIZE);
        for (u32 offset = 0; offset < PAGE_SIZE; offset += block_size) {
            u64 pos = page_pos + offset;
            sector_t sector;
            struct bio_vec *last;
            bool extend;
            int disk_block;

            if (pos >= file_size) break;
            disk_block = ext2_block_mapping(inode, pos / block_size);
            if (disk_block < 0) {
                ret = disk_block;
                goto out;
            }
            // 文件块未分配，提交前面已收集的批次
            if (!disk_block) {
                ret = ext2_read_batch(bio);
                if (ret < 0)
                    goto out;
                continue;
            }

            sector = (u64)disk_block * block_size / SECTOR_SIZE;
            last = bio->bi_vcnt ? &bio->bi_io_vec[bio->bi_vcnt - 1] : NULL;
            // 如果当前块与上一个块连续，且 BIO 还没满，则合并到同一个 BIO
            extend = last && (last->page == page) && (last->offset + last->len == offset);

            // 如果当前块与 BIO 中的最后一个块不连续，或者 BIO 已满，则提交当前 BIO
            if (bio->bi_size && (sector != bio->bi_sector + bio->bi_size / SECTOR_SIZE || bio->bi_size > max_bytes - block_size || (!extend && bio->bi_vcnt == bio->bi_max_vecs))) {
                ret = ext2_read_batch(bio);
                if (ret < 0)
                    goto out;
                extend = false;
            }

            // 如果 BIO 为空，则设置 BIO 的起始扇区
            if (!bio->bi_size) {
                bio->bi_sector = sector;
            }

            // 合并
            if (extend) {
                last->len += block_size;
                bio->bi_size += block_size;
            } else {
                ret = bio_add_page(bio, page, block_size, offset);
                if (ret < 0)
                    goto out;
            }
        }
    }
    // 防止循环结束h后还有未提交的 BIO
    ret = ext2_read_batch(bio);
    if (ret == 0) {
        for (u32 i = 0; i < nr_pages; i++) {
            u64 pos = (u64)pages[i]->index * PAGE_SIZE;
            if (pos < file_size && file_size - pos < PAGE_SIZE) {
                size_t valid = file_size - pos;
                memset((u8 *)pagecache_data(pages[i]) + valid, 0, PAGE_SIZE - valid);
            }
        }
    }
out:
    bio_put(bio);
    return ret;
}

static int ext2_readpage(struct page *page)
{
    return ext2_readpages(&page, 1);
}

static int ext2_writepage(struct page *page)
{
    return ext2_rwpage(page, true);
}

static int ext2_commit_dir_page(struct inode *dir, struct page *page) {
    int ret;

    SetPageDirty(page);

    ret = pagecache_write_page(page);
    if (ret < 0)
        return ret;

    ext2_put_page(page);

    return ext2_write_inode(dir);
}

const struct address_space_operations ext2_aops = {
    .readpage = ext2_readpage,
    .readpages = ext2_readpages,
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
        if (valid_bytes_in_page < rec_len) {
            ext2_put_page(page);
            continue;
        }
        u32 end = valid_bytes_in_page - rec_len;
        u32 offset = 0;
        while (offset <= end) {
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

    return 0;
}

static void ext2_init_dir_entry(struct ext2_dir_entry_2 *de, u32 ino, u16 mode, const char *name, u32 name_len, u16 rec_len) {
    de->inode = ino;
    de->rec_len = rec_len;
    de->name_len = name_len;
    de->file_type = ext2_mode_to_ftype(mode);
    memcpy(de->name, name, name_len);
}

static timespec_t ext2_now(void) {
    u64 now = monotonic_ns();

    return (timespec_t) {
        .tv_sec = now / NSEC_PER_SEC,
        .tv_nsec = now % NSEC_PER_SEC,
    };
}

static int ext2_delete_entry(struct inode *dir, const struct qstr *child) {
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (u32 i = 0; i < page_nr; i++) {
        struct page *page = ext2_get_page(dir, i);
        char *buf;
        u32 valid_end;
        u32 offset = 0;
        struct ext2_dir_entry_2 *prev = NULL;

        if (IS_ERR(page))
            return PTR_ERR(page);

        buf = (char *)page_address(page);
        valid_end = last_valid_byte(dir, i);

        while (offset < valid_end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);

            if (entry->rec_len == 0) {
                ext2_put_page(page);
                return -EIO;
            }

            if (entry->inode != 0 && entry->name_len == child->len &&
                memcmp(entry->name, child->name, child->len) == 0) {
                if (prev != NULL) {
                    prev->rec_len += entry->rec_len;
                } else {
                    entry->inode = 0;
                }

                SetPageDirty(page);
                return ext2_commit_dir_page(dir, page);
            }

            prev = entry;
            offset += entry->rec_len;
        }

        ext2_put_page(page);
    }

    return -ENOENT;
}

static int ext2_dir_is_empty(struct inode *dir) {
    u32 page_nr = (dir->i_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (u32 i = 0; i < page_nr; i++) {
        struct page *page = ext2_get_page(dir, i);
        char *buf;
        u32 valid_end;
        u32 offset = 0;

        if (IS_ERR(page))
            return PTR_ERR(page);

        buf = (char *)page_address(page);
        valid_end = last_valid_byte(dir, i);

        while (offset < valid_end) {
            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(buf + offset);

            if (entry->rec_len == 0) {
                ext2_put_page(page);
                return -EIO;
            }

            if (entry->inode != 0 &&
                !((entry->name_len == 1 && entry->name[0] == '.') ||
                  (entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.'))) {
                ext2_put_page(page);
                return 0;
            }

            offset += entry->rec_len;
        }

        ext2_put_page(page);
    }

    return 1;
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
    ext2_init_dir_entry(dot, new_dir->i_ino, S_IFDIR, ".", 1, dot_rec_len);

    dotdot = (struct ext2_dir_entry_2 *)(buf + dot_rec_len);
    ext2_init_dir_entry(dotdot, parent_ino, S_IFDIR, "..", 2,
                        block_size - dot_rec_len);
    SetPageDirty(page);
    ret = pagecache_write_page(page); // 立即写回
    if (ret < 0) {
        ext2_put_page(page);
        return ret;
    }
    
    ext2_put_page(page);

    new_dir->i_size = block_size;
    return ext2_write_inode(new_dir);
}

static int ext2_add_entry_to_slot(struct inode *parent, const char *name, u32 name_len, 
                                                u32 ino, u16 mode, const dir_slot_t *slot) {
    struct page *page;
    char *buf;
    struct ext2_dir_entry_2 *new_entry;
    int ret;

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
    ret = pagecache_write_page(page); // 立即写回

    ext2_put_page(page);
    if (ret < 0)
        return ret;

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

static int ext2_write_symlink_target(struct inode *inode, const char *target)
{
    size_t len;
    struct page *page;
    char *buf;
    int ret;

    if (inode == NULL || target == NULL)
        return -EINVAL;

    len = strlen(target);
    if (len >= PAGE_SIZE)
        return -ENAMETOOLONG;

    ret = ext2_block_set_mapping(inode, 0);
    if (ret < 0)
        return ret;

    page = ext2_get_page(inode, 0);
    if (IS_ERR(page))
        return PTR_ERR(page);

    buf = page_address(page);
    memset(buf, 0, PAGE_SIZE);
    memcpy(buf, target, len);

    SetPageUptodate(page);
    SetPageDirty(page);
    ret = pagecache_write_page(page);
    ext2_put_page(page);
    if (ret < 0)
        return ret;

    inode->i_size = len;
    return ext2_write_inode(inode);
}

static int ext2_symlink(struct inode *dir, struct dentry *dentry, const char *target)
{
    struct inode *inode;
    int ret;

    if (dir == NULL || dentry == NULL || target == NULL)
        return -EINVAL;

    inode = ext2_new_inode(dir, S_IFLNK | 0777);
    if (IS_ERR(inode))
        return PTR_ERR(inode);

    ret = ext2_write_symlink_target(inode, target);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, inode->i_ino);
        return ret;
    }

    d_add(dentry, inode);

    ret = ext2_add_entry(dir, dentry->d_name.name, dentry->d_name.len,
                         inode->i_ino, S_IFLNK);
    if (ret < 0) {
        ext2_release_ino(dir->i_sb, inode->i_ino);
        dentry->d_inode = NULL;
        return ret;
    }

    dir->i_mtime = dir->i_ctime;
    ext2_write_inode(dir);

    return 0;
}

static int ext2_unlink(struct inode *dir, struct dentry *dentry) {
    struct inode *inode;
    struct ext2_inode_info *ei;
    timespec_t now;
    int ret;

    if (dir == NULL || dentry == NULL || dentry->d_inode == NULL)
        return -EINVAL;

    inode = dentry->d_inode;
    if (S_ISDIR(inode->i_mode))
        return -EISDIR;

    ret = ext2_delete_entry(dir, &dentry->d_name);
    if (ret < 0)
        return ret;

    now = ext2_now();
    dir->i_mtime = dir->i_ctime = now;
    ext2_write_inode(dir);

    if (inode->i_nlink > 0)
        inode->i_nlink--;
    inode->i_ctime = inode->i_mtime = now;
    ei = EXT2_I(inode);
    if (inode->i_nlink == 0) {
        ret = ext2_free_inode_blocks(inode);
        if (ret < 0)
            return ret;
        ei->i_dtime = now.tv_sec;
        inode->i_size = 0;
    }
    ret = ext2_write_inode(inode);
    if (ret < 0)
        return ret;
    if (inode->i_nlink == 0) {
        ret = ext2_release_ino(inode->i_sb, inode->i_ino);
        if (ret < 0)
            return ret;
    }

    d_add(dentry, NULL);
    return 0;
}

static int ext2_rmdir(struct inode *dir, struct dentry *dentry) {
    struct inode *inode;
    struct ext2_inode_info *ei;
    timespec_t now;
    int ret;

    if (dir == NULL || dentry == NULL || dentry->d_inode == NULL)
        return -EINVAL;

    inode = dentry->d_inode;
    if (!S_ISDIR(inode->i_mode))
        return -ENOTDIR;

    ret = ext2_dir_is_empty(inode);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return -ENOTEMPTY;

    ret = ext2_delete_entry(dir, &dentry->d_name);
    if (ret < 0)
        return ret;

    now = ext2_now();
    if (dir->i_nlink > 0)
        dir->i_nlink--;
    dir->i_mtime = dir->i_ctime = now;
    ext2_write_inode(dir);

    inode->i_nlink = 0;
    inode->i_ctime = inode->i_mtime = now;
    ei = EXT2_I(inode);
    ret = ext2_free_inode_blocks(inode);
    if (ret < 0)
        return ret;
    ei->i_dtime = now.tv_sec;
    inode->i_size = 0;
    ret = ext2_write_inode(inode);
    if (ret < 0)
        return ret;
    ret = ext2_release_ino(inode->i_sb, inode->i_ino);
    if (ret < 0)
        return ret;

    d_add(dentry, NULL);
    return 0;
}

static int ext2_rename(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry) {
    struct inode *inode;
    timespec_t now;
    int ret;

    if (old_dir == NULL || old_dentry == NULL || new_dir == NULL ||
        new_dentry == NULL || old_dentry->d_inode == NULL)
        return -EINVAL;

    inode = old_dentry->d_inode;
    if (new_dentry->d_inode != NULL)
        return -EEXIST;

    ret = ext2_add_entry(new_dir, new_dentry->d_name.name,
                         new_dentry->d_name.len, inode->i_ino,
                         inode->i_mode);
    if (ret < 0)
        return ret;

    ret = ext2_delete_entry(old_dir, &old_dentry->d_name);
    if (ret < 0)
        return ret;

    now = ext2_now();
    old_dir->i_mtime = old_dir->i_ctime = now;
    new_dir->i_mtime = new_dir->i_ctime = now;
    inode->i_ctime = now;

    ret = ext2_write_inode(old_dir);
    if (ret < 0)
        return ret;

    if (new_dir != old_dir) {
        ret = ext2_write_inode(new_dir);
        if (ret < 0)
            return ret;
    }

    return ext2_write_inode(inode);
}

const struct inode_operations ext2_dir_inode_operations = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .mkdir = ext2_mkdir,
    .mknod = ext2_mknod,
    .unlink = ext2_unlink,
    .rmdir = ext2_rmdir,
    .rename = ext2_rename,
    .symlink = ext2_symlink,
};
