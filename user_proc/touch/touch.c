#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

static int touch_one(const char *path)
{
    struct stat st;
    int fd;

    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        errno = EISDIR;
        return -1;
    }

    fd = open(path, O_RDONLY | O_CREAT, 0644);
    if (fd < 0) {
        return -1;
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int i;

    if (argc < 2) {
        printf("usage: touch file...\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (touch_one(argv[i]) < 0) {
            printf("touch: cannot touch %s, errno=%d\n",
                   argv[i],
                   errno);
            ret = 1;
        }
    }

    return ret;
}
