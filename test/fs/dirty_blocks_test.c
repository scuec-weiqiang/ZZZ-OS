/* OpenAI Codex 生成：独立模拟测试，不接入内核启动，不访问实际磁盘。 */
#include "../../fs/ext2/ext2_dir.c"

extern int puts(const char *s);

static u8 data[PAGE_SIZE];
static int disk_blocks[8];
static u32 requests;
static u64 write_pos[8];
static size_t write_len[8];
static int fail_request;

void *page_address(struct page *page)
{
    return data;
}

int ext2_block_mapping(struct inode *inode, u32 index)
{
    return disk_blocks[index];
}

int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos)
{
    return -EIO;
}

int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos)
{
    write_pos[requests] = pos;
    write_len[requests++] = len;
    return fail_request == requests ? -EIO : 0;
}

#define CHECK_TEST(expr) do { if (!(expr)) { puts("FAIL: " #expr); return 1; } } while (0)

int main(void)
{
    struct blkdev bdev = {0};
    struct super_block sb = {.s_blocksize = 1024, .s_bdev = &bdev};
    struct inode inode = {.i_sb = &sb};
    struct address_space mapping = {.host = &inode};
    struct page page = {.mapping = &mapping};

    CHECK_TEST(pagecache_mark_dirty_range(&page, 900, 300, 1024) == 0);
    CHECK_TEST(page.dirty_blocks == 3 && PageDirty(&page));
    CHECK_TEST(pagecache_mark_dirty_range(&page, 3072, 1, 1024) == 0);
    CHECK_TEST(page.dirty_blocks == 11);
    CHECK_TEST(pagecache_mark_dirty_range(&page, PAGE_SIZE, 1, 1024) == -EINVAL);
    CHECK_TEST(pagecache_mark_dirty_range(&page, 0, 1, 256) == -EINVAL);
    CHECK_TEST(page.dirty_blocks == 11);

    ClearPageDirty(&page);
    CHECK_TEST(page.dirty_blocks == 0 && !PageDirty(&page));
    CHECK_TEST(pagecache_mark_dirty_range(&page, 0, 0, 1024) == 0);
    CHECK_TEST(!PageDirty(&page));
    CHECK_TEST(pagecache_mark_dirty_range(&page, 0, PAGE_SIZE, 512) == 0);
    CHECK_TEST(page.dirty_blocks == 255);
    ClearPageDirty(&page);
    CHECK_TEST(pagecache_mark_dirty_range(&page, 4095, 1, 4096) == 0);
    CHECK_TEST(page.dirty_blocks == 1);

    SetPageDirty(&page);
    CHECK_TEST(pagecache_mark_dirty_range(&page, 1024, 1, 1024) == 0);
    CHECK_TEST(page.dirty_blocks == 0 && PageDirty(&page));

    disk_blocks[0] = 100;
    disk_blocks[1] = 101;
    disk_blocks[2] = 102;
    disk_blocks[3] = 103;
    ClearPageDirty(&page);
    CHECK_TEST(pagecache_mark_dirty_range(&page, 0, 2048, 1024) == 0);
    CHECK_TEST(pagecache_mark_dirty_range(&page, 3072, 1, 1024) == 0);
    CHECK_TEST(ext2_rwpage(&page, true) == 0);
    CHECK_TEST(requests == 2);
    CHECK_TEST(write_pos[0] == 100 * 1024 && write_len[0] == 2048);
    CHECK_TEST(write_pos[1] == 103 * 1024 && write_len[1] == 1024);

    requests = 0;
    fail_request = 2;
    CHECK_TEST(ext2_rwpage(&page, true) == -EIO);
    CHECK_TEST(page.dirty_blocks == 11 && PageDirty(&page));
    fail_request = 0;
    disk_blocks[3] = 0;
    CHECK_TEST(ext2_rwpage(&page, true) == -EIO);

    requests = 0;
    SetPageDirty(&page);
    CHECK_TEST(ext2_rwpage(&page, true) == 0);
    CHECK_TEST(requests == 1 && write_len[0] == 3072);
    puts("dirty-blocks-test: PASS");
    return 0;
}
