/* OpenAI Codex 生成：读取入口模拟测试，不访问磁盘。 */
#include "../../fs/ext2/ext2_file.c"

extern int puts(const char *s);
static struct page cached_page;
static u8 page_data[PAGE_SIZE];
static u8 buffer[PAGE_SIZE * 3 + 2];
static u32 calls;
static bool allowed[8];
static int fail_at;

int pagecache_read_page(struct address_space *mapping, pgoff_t index,
                        bool allow_readahead, struct page **page_out)
{
    allowed[calls++] = allow_readahead;
    if (!mapping->host->i_size ||
        index > (mapping->host->i_size - 1) / PAGE_SIZE)
        return -EINVAL;
    if (calls == fail_at)
        return -EIO;
    memset(page_data, index + 1, sizeof(page_data));
    *page_out = &cached_page;
    return 0;
}

void *page_address(struct page *page) { return page_data; }
void ext2_put_page(struct page *page) { }

#define CHECK_TEST(expr) do { if (!(expr)) { puts("FAIL: " #expr); return 1; } } while (0)

int main(void)
{
    struct inode inode = {.i_size = PAGE_SIZE * 2 + 17};
    struct address_space mapping = {.host = &inode};
    struct file file = {.f_inode = &inode};
    loff_t pos = 0;
    ssize_t ret;

    inode.i_mapping = &mapping;
    memset(buffer, 0xa5, sizeof(buffer));
    ret = ext2_file_read(&file, (char *)buffer + 1, sizeof(buffer) - 2, &pos);
    CHECK_TEST(ret == inode.i_size && pos == inode.i_size);
    CHECK_TEST(calls == 3 && !allowed[0] && allowed[1] && allowed[2]);
    CHECK_TEST(buffer[0] == 0xa5 && buffer[ret + 1] == 0xa5);
    for (u32 i = 0; i < ret; i++)
        CHECK_TEST(buffer[i + 1] == i / PAGE_SIZE + 1);
    CHECK_TEST(file.f_ra.valid && file.f_ra.prev_end == pos);
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, 1, &pos) == 0);
    CHECK_TEST(calls == 3);

    calls = 0;
    pos = PAGE_SIZE - 1;
    ret = ext2_file_read(&file, (char *)buffer, 2, &pos);
    CHECK_TEST(ret == 2 && calls == 2 && !allowed[0] && allowed[1]);
    CHECK_TEST(buffer[0] == 1 && buffer[1] == 2);
    calls = 0;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, 1, &pos) == 1);
    CHECK_TEST(allowed[0]);

    calls = 0;
    pos = 0;
    file.f_ra.disabled = true;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, PAGE_SIZE + 1, &pos) == PAGE_SIZE + 1);
    CHECK_TEST(!allowed[0] && !allowed[1]);

    calls = 0;
    fail_at = 1;
    pos = 0;
    file.f_ra.valid = false;
    file.f_ra.prev_end = 123;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, 10, &pos) == -EIO);
    CHECK_TEST(pos == 0 && !file.f_ra.valid && file.f_ra.prev_end == 123);

    calls = 0;
    fail_at = 2;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, PAGE_SIZE + 1, &pos) == PAGE_SIZE);
    CHECK_TEST(pos == PAGE_SIZE && file.f_ra.prev_end == PAGE_SIZE);

    calls = 0;
    pos = -1;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, 1, &pos) == -EINVAL);
    pos = 0;
    inode.i_size = 0;
    CHECK_TEST(ext2_file_read(&file, (char *)buffer, 1, &pos) == 0);
    CHECK_TEST(calls == 0);
    puts("file-read-entry-test: PASS");
    return 0;
}
