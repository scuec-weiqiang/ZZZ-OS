#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

static int write_all(int fd, const char *buf, int len)
{
    int off = 0;

    while (off < len) {
        int n = write(fd, buf + off, len - off);

        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        off += n;
    }

    return 0;
}

static int cat_fd(int fd, const char *name)
{
    char buf[512];
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write_all(STDOUT_FILENO, buf, n) < 0) {
            printf("cat: write failed for %s, errno=%d\n",
                   name ? name : "<stdin>",
                   errno);
            return 1;
        }
    }

    if (n < 0) {
        printf("cat: read failed for %s, errno=%d\n",
               name ? name : "<stdin>",
               errno);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int ret = 0;
    int i;

    if (argc < 2) {
        return cat_fd(STDIN_FILENO, NULL);
    }

    for (i = 1; i < argc; i++) {
        int fd;

        if (argv[i][0] == '-' && argv[i][1] == '\0') {
            if (cat_fd(STDIN_FILENO, NULL) != 0) {
                ret = 1;
            }
            continue;
        }

        fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            printf("cat: cannot open %s, errno=%d\n",
                   argv[i],
                   errno);
            ret = 1;
            continue;
        }

        if (cat_fd(fd, argv[i]) != 0) {
            ret = 1;
        }

        close(fd);
    }

    return ret;
}
