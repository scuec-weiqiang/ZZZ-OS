/* OpenAI Codex 生成：只允许在独立测试镜像上运行的用户态写入测试。 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ROOT "/codex-ext2-write-test"
#define LENGTH 10100
static unsigned char expected[LENGTH];
static unsigned char actual[LENGTH + 1];
static int small_reads;

static int verify_file(const char *path, const unsigned char *data, size_t length)
{
    int fd = open(path, O_RDONLY);
    size_t done = 0;
    if (fd < 0) { perror(path); return -1; }
    while (done < length) {
        size_t chunk = length - done;
        if (small_reads && chunk > 512) chunk = 512;
        ssize_t n = read(fd, actual + done, chunk);
        if (n <= 0) { printf("FAIL read %s offset=%lu\n", path, (unsigned long)done); close(fd); return -1; }
        done += n;
    }
    if (memcmp(actual, data, length) || read(fd, actual + length, 1) != 0) {
        printf("FAIL contents/EOF %s\n", path); close(fd); return -1;
    }
    close(fd);
    return 0;
}

static int write_exact(int fd, const void *data, size_t length)
{
    size_t done = 0;
    while (done < length) {
        ssize_t n = write(fd, (const char *)data + done, length - done);
        if (n <= 0) { perror("write"); return -1; }
        done += n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int verify = argc >= 2 && !strcmp(argv[1], "verify");
    int fd;
    char path[128];
    unsigned char patch[2500];
    static unsigned char sparse[8216];

    if (!verify && !(argc >= 2 && !strcmp(argv[1], "create"))) {
        puts("usage: ext2-disk-write-test create|verify [small-reads] (DISPOSABLE IMAGE ONLY)");
        return 1;
    }
    small_reads = argc == 3 && !strcmp(argv[2], "small-reads");
    for (size_t i = 0; i < LENGTH; i++) expected[i] = (i * 37 + 11) & 255;
    memset(patch, 0xc7, sizeof(patch));
    memset(sparse + 8199, 0x6d, 17);

    if (!verify) {
        if (mkdir(ROOT, 0755) || mkdir(ROOT "/nested", 0755) ||
            mkdir(ROOT "/nested/deep", 0755)) { perror("mkdir"); return 1; }
        puts("PASS mkdir nested");
        fd = open(ROOT "/nested/deep/data", O_CREAT | O_EXCL | O_RDWR, 0644);
        if (fd < 0) { perror("create data"); return 1; }
        if (write_exact(fd, expected, 100)) return 1;
        close(fd);
        if (verify_file(ROOT "/nested/deep/data", expected, 100)) return 1;
        puts("PASS empty-file write 100 bytes");
        fd = open(ROOT "/nested/deep/data", O_RDWR);
        if (fd < 0 || lseek(fd, 0, SEEK_END) != 100 ||
            write_exact(fd, expected + 100, LENGTH - 100)) return 1;
        close(fd);
        if (verify_file(ROOT "/nested/deep/data", expected, LENGTH)) return 1;
        puts("PASS cross-page append 10000 bytes");
        fd = open(ROOT "/nested/deep/data", O_RDWR);
        if (fd < 0 || lseek(fd, 1000, SEEK_SET) != 1000 ||
            write_exact(fd, patch, sizeof(patch))) return 1;
        close(fd);
        fd = open(ROOT "/sparse", O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd < 0 || lseek(fd, 8199, SEEK_SET) != 8199 ||
            write_exact(fd, sparse + 8199, 17)) return 1;
        close(fd);
        for (unsigned i = 0; i < 100; i++) {
            snprintf(path, sizeof(path), ROOT "/nested/entry-%03u", i);
            fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
            if (fd < 0 || write_exact(fd, &expected[i], 1)) return 1;
            close(fd);
        }
        puts("PASS create 100 directory entries");
    }
    /* entry 数据在覆盖主文件之前的预期序列中取值。 */
    for (unsigned i = 0; i < 100; i++) {
        snprintf(path, sizeof(path), ROOT "/nested/entry-%03u", i);
        if (verify_file(path, &expected[i], 1)) return 1;
    }
    memcpy(expected + 1000, patch, sizeof(patch));
    if (verify_file(ROOT "/nested/deep/data", expected, LENGTH)) return 1;
    puts("PASS partial overwrite and preserved bytes");
    if (verify_file(ROOT "/sparse", sparse, sizeof(sparse))) return 1;
    puts("PASS sparse gap zero-fill");
    printf("ext2-disk-write-test: PASS mode=%s read-mode=%s\n",
           verify ? "verify" : "create", small_reads ? "512-byte" : "large");
    return 0;
}
