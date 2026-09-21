/* OpenAI Codex 生成：多页读取模拟测试，不访问真实磁盘。 */
#include "../../fs/ext2/ext2_dir.c"

extern int puts(const char *s);
static u8 data[20][PAGE_SIZE];
static int blocks[80];
static struct bio test_bio;
static struct bio_vec vecs[16];
static u32 requests;
static u32 sizes[80];
static int fail_at;
static u32 mapping_calls;
static intptr_t panic_env[5];

void panic(const char *fmt, ...)
{
    __builtin_longjmp(panic_env, 1);
}

phys_addr_t page_to_phys(struct page *page)
{
    return KERNEL_PA(data[page->index]);
}

int ext2_block_mapping(struct inode *inode, u32 index)
{
    mapping_calls++;
    return blocks[index];
}

struct bio *bio_alloc(u32 nr_vecs)
{
    memset(&test_bio, 0, sizeof(test_bio));
    test_bio.bi_io_vec = vecs;
    test_bio.bi_max_vecs = nr_vecs;
    return &test_bio;
}

void bio_put(struct bio *bio) { }

int bio_add_page(struct bio *bio, struct page *page, u32 len, u32 offset)
{
    if (bio->bi_vcnt == bio->bi_max_vecs || offset + len > PAGE_SIZE)
        return -EINVAL;
    bio->bi_io_vec[bio->bi_vcnt++] = (struct bio_vec){page, offset, len};
    bio->bi_size += len;
    return 0;
}

int submit_bio_wait(struct bio *bio)
{
    u32 bytes = 0;

    if (bio->bi_sector >= bio->bi_bdev->bd_nr_sectors ||
        bio->bi_size / SECTOR_SIZE > bio->bi_bdev->bd_nr_sectors - bio->bi_sector)
        return -EIO;
    sizes[requests++] = bio->bi_size;
    if (requests == fail_at)
        return -EIO;
    for (u32 i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *vec = &bio->bi_io_vec[i];
        if (vec->offset + vec->len > PAGE_SIZE)
            return -EINVAL;
        memset(data[vec->page->index] + vec->offset, 0x5a, vec->len);
        bytes += vec->len;
    }
    return bytes == bio->bi_size ? 0 : -EINVAL;
}

#define CHECK_TEST(expr) do { if (!(expr)) { puts("FAIL: " #expr); return 1; } } while (0)
#define EXPECT_ASSERT(expr) do { \
    if (__builtin_setjmp(panic_env) == 0) { \
        (void)(expr); \
        puts("FAIL: expected ASSERT: " #expr); \
        return 1; \
    } \
} while (0)

int main(void)
{
    struct request_queue queue = {.logical_block_size = 512, .max_hw_sectors = 256};
    struct gendisk disk = {.queue = &queue};
    struct blkdev bdev = {.bd_disk = &disk, .bd_nr_sectors = 10000};
    struct super_block sb = {.s_bdev = &bdev, .s_blocksize = 1024};
    struct inode inode = {.i_sb = &sb, .i_size = sizeof(data)};
    struct address_space mapping = {.host = &inode};
    struct page page[20] = {0};
    struct page *pages[20];
    int evaluated = 0;

    ASSERT(++evaluated == 1, "assert condition must run once");
    CHECK_TEST(evaluated == 1);

    for (u32 i = 0; i < 20; i++) {
        page[i].mapping = &mapping;
        page[i].index = i;
        pages[i] = &page[i];
    }
    for (u32 i = 0; i < 80; i++)
        blocks[i] = 100 + i;

    CHECK_TEST(ext2_readpages(NULL, 0) == 0);
    EXPECT_ASSERT(ext2_readpages(NULL, 1));
    CHECK_TEST(ext2_readpages(pages, 20) == 0);
    CHECK_TEST(requests == 2 && sizes[0] == 16 * PAGE_SIZE && sizes[1] == 4 * PAGE_SIZE);

    requests = 0;
    queue.max_hw_sectors = 4;
    CHECK_TEST(ext2_readpages(pages, 2) == 0);
    CHECK_TEST(requests == 4);
    for (u32 i = 0; i < requests; i++)
        CHECK_TEST(sizes[i] == 2048);

    queue.max_hw_sectors = 256;
    inode.i_size = PAGE_SIZE + 17;
    blocks[1] = 0;
    requests = 0;
    CHECK_TEST(ext2_readpages(pages, 2) == 0);
    CHECK_TEST(requests == 2);
    CHECK_TEST(data[0][0] == 0x5a && data[0][1024] == 0);
    CHECK_TEST(data[1][16] == 0x5a && data[1][17] == 0 && data[1][4095] == 0);
    CHECK_TEST(!PageUptodate(&page[0]));

    requests = 0;
    fail_at = 2;
    CHECK_TEST(ext2_readpages(pages, 2) == -EIO);
    CHECK_TEST(!PageUptodate(&page[0]) && !PageUptodate(&page[1]));
    fail_at = 0;

    requests = 0;
    SetPageDirty(&page[1]);
    data[0][0] = 0xaa;
    EXPECT_ASSERT(ext2_readpages(pages, 2));
    CHECK_TEST(data[0][0] == 0xaa && requests == 0);
    ClearPageDirty(&page[1]);
    SetPageUptodate(&page[1]);
    EXPECT_ASSERT(ext2_readpages(pages, 2));
    ClearPageUptodate(&page[1]);
    memset(data[2], 0xaa, PAGE_SIZE);
    CHECK_TEST(ext2_readpages(pages, 3) == 0);
    for (u32 i = 0; i < PAGE_SIZE; i++)
        CHECK_TEST(data[2][i] == 0);
    pages[1] = pages[0];
    EXPECT_ASSERT(ext2_readpages(pages, 2));
    pages[1] = &page[1];
    page[1].mapping = NULL;
    EXPECT_ASSERT(ext2_readpages(pages, 2));
    page[1].mapping = &mapping;

    blocks[0] = 5000;
    CHECK_TEST(ext2_readpages(pages, 1) == -EIO);
    blocks[0] = -EIO;
    CHECK_TEST(ext2_readpages(pages, 1) == -EIO);

    /* EOF 外即使存在磁盘映射，也不应读取。 */
    inode.i_size = 0;
    requests = 0;
    mapping_calls = 0;
    memset(data, 0xaa, sizeof(data));
    CHECK_TEST(ext2_readpage(&page[0]) == 0);
    CHECK_TEST(ext2_readpages(pages + 1, 2) == 0);
    CHECK_TEST(requests == 0 && mapping_calls == 0);
    for (u32 p = 0; p < 3; p++)
        for (u32 i = 0; i < PAGE_SIZE; i++)
            CHECK_TEST(data[p][i] == 0);

    inode.i_size = PAGE_SIZE;
    memset(data[1], 0xaa, PAGE_SIZE);
    CHECK_TEST(ext2_readpage(&page[1]) == 0);
    CHECK_TEST(requests == 0 && mapping_calls == 0);
    for (u32 i = 0; i < PAGE_SIZE; i++)
        CHECK_TEST(data[1][i] == 0);

    inode.i_size = 17;
    blocks[0] = 100;
    memset(data[0], 0xaa, PAGE_SIZE);
    CHECK_TEST(ext2_readpage(&page[0]) == 0);
    CHECK_TEST(requests == 1 && mapping_calls == 1);
    for (u32 i = 0; i < PAGE_SIZE; i++)
        CHECK_TEST(data[0][i] == (i < 17 ? 0x5a : 0));
    puts("ext2-readpages-test: PASS");
    return 0;
}
