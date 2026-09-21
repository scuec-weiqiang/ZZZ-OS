/* OpenAI Codex 生成：4 KiB Ext2 独立镜像上的用户态块计数测试。 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static unsigned char data[4096];
static int check_count(const char *path, long expected, long size)
{
    struct stat st;
    int fd = open(path, O_RDONLY);
    if (fd < 0 || fstat(fd, &st)) { perror(path); return -1; }
    close(fd);
    printf("blocks-test: %s size=%ld blocks=%ld expected=%ld\n",
           path, (long)st.st_size, (long)st.st_blocks, expected);
    return st.st_blocks == expected && st.st_size == size ? 0 : -1;
}

int main(int argc, char **argv)
{
    const char *single = "/codex-ext2-write-test/single";
    const char *double_file = "/codex-ext2-write-test/double-sparse";
    const char *deleted = "/codex-ext2-write-test/delete-indirect";
    int verify = argc == 2 && !strcmp(argv[1], "verify");
    long double_pos = (12L + 1024) * 4096 + 7;
    int fd;

    if (!verify && !(argc == 2 && !strcmp(argv[1], "create"))) return 1;
    memset(data, 0x39, sizeof(data));
    if (!verify) {
        fd = open(single, O_CREAT | O_EXCL | O_RDWR, 0644);
        if (fd < 0) return 1;
        for (int i = 0; i < 16; i++)
            if (write(fd, data, sizeof(data)) != sizeof(data)) return 1;
        if (write(fd, data, 1) != 1) return 1;
        if (lseek(fd, 0, SEEK_SET) != 0 || write(fd, data, 100) != 100) return 1;
        close(fd);
        fd = open(double_file, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd < 0 || lseek(fd, double_pos, SEEK_SET) != double_pos || write(fd, data, 1) != 1) return 1;
        close(fd);
        fd = open(deleted, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd < 0) return 1;
        for (int i = 0; i < 13; i++)
            if (write(fd, data, sizeof(data)) != sizeof(data)) return 1;
        close(fd);
        if (check_count(deleted, 112, 13 * 4096) || unlink(deleted)) return 1;
    }
    if (check_count("/codex-ext2-write-test/nested/deep/data", 24, 10100) ||
        check_count("/codex-ext2-write-test/sparse", 8, 8216) ||
        check_count("/codex-ext2-write-test/nested/entry-000", 8, 1) ||
        check_count(single, 144, 65537) || check_count(double_file, 24, double_pos + 1)) return 1;
    fd = open(deleted, O_RDONLY);
    if (fd >= 0) { close(fd); return 1; }
    puts("ext2-disk-blocks-test: PASS");
    return 0;
}
