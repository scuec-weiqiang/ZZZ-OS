/* OpenAI Codex 生成：运行真实块映射代码，模拟块分配和块 I/O。 */
#include "../../fs/ext2/ext2_blk.c"
extern void *malloc(size_t size);
extern void free(void *ptr);
extern int puts(const char *s);
static u8 disk_data[64][4096];
static bool allocated[64];
static bool fail_alloc;
static bool fail_free;

void *kmalloc(size_t size) { return malloc(size); }
void kfree(void *ptr) { free(ptr); }
u32 ext2_alloc_bno(struct super_block *sb)
{
    if (fail_alloc) return 0;
    for (u32 i = 1; i < 64; i++)
        if (!allocated[i]) { allocated[i] = true; return i; }
    return 0;
}
int ext2_release_bno(struct super_block *sb, u32 bno)
{
    if (fail_free || !allocated[bno]) return -EIO;
    allocated[bno] = false;
    return 0;
}
int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos)
{
    memcpy(buf, disk_data[pos / 4096], len); return 0;
}
int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos)
{
    memcpy(disk_data[pos / 4096], buf, len); return 0;
}
int ext2_indirect_cache_write(struct super_block *sb, u32 bno, const void *data)
{
    memcpy(disk_data[bno], data, 4096); return 0;
}
void ext2_indirect_cache_invalidate(struct super_block *sb, u32 bno) { }
int pagecache_sync_mapping(struct address_space *mapping) { return 0; }
int pagecache_invalidate_mapping(struct address_space *mapping) { return 0; }

#define TEST(expr) do { if (!(expr)) { puts("FAIL: " #expr); return 1; } } while (0)
int main(void)
{
    struct super_block sb = {.s_blocksize = 4096};
    struct ext2_inode_info ei = {0};
    struct inode *inode = &ei.vfs_inode;
    u32 indexes[] = {0, 12, 12 + 1024, 12 + 1024 + 1024 * 1024};
    u64 counts[] = {8, 24, 48, 80};
    inode->i_sb = &sb;
    for (u32 i = 0; i < 4; i++) {
        int bno = ext2_block_set_mapping(inode, indexes[i]);
        TEST(bno > 0);
        TEST(inode->i_blocks == counts[i]);
        TEST(ext2_block_set_mapping(inode, indexes[i]) == bno);
        TEST(inode->i_blocks == counts[i]);
    }
    fail_alloc = true;
    TEST(ext2_block_set_mapping(inode, 1) == -ENOSPC);
    TEST(inode->i_blocks == 80);
    fail_alloc = false;
    fail_free = true;
    TEST(ext2_free_inode_blocks(inode) == -EIO);
    TEST(inode->i_blocks == 80);
    fail_free = false;
    TEST(ext2_free_inode_blocks(inode) == 0);
    TEST(inode->i_blocks == 0);
    for (u32 i = 1; i < 64; i++) TEST(!allocated[i]);
    TEST(ext2_free_inode_blocks(inode) == 0);
    TEST(inode->i_blocks == 0);
    puts("ext2-blocks-test: PASS direct/single/double/triple/free/failure");
    return 0;
}
