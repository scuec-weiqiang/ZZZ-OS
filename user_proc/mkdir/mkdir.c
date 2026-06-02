#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MKDIR_PATH_MAX 256

static int ensure_dir(const char *path, int create_parents)
{
    char tmp[MKDIR_PATH_MAX];
    size_t len;
    size_t i;

    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    len = strlen(path);
    if (len >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(tmp, path, len + 1);

    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    if (!create_parents) {
        return mkdir(tmp, 0755);
    }

    for (i = (tmp[0] == '/') ? 1 : 0; i <= len; i++) {
        struct stat st;
        char saved;

        if (tmp[i] != '/' && tmp[i] != '\0') {
            continue;
        }

        saved = tmp[i];
        tmp[i] = '\0';

        if (tmp[0] != '\0') {
            if (stat(tmp, &st) < 0) {
                if (mkdir(tmp, 0755) < 0) {
                    tmp[i] = saved;
                    return -1;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                tmp[i] = saved;
                errno = ENOTDIR;
                return -1;
            }
        }

        tmp[i] = saved;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int create_parents = 0;
    int ret = 0;
    int i = 1;

    if (argc < 2) {
        printf("usage: mkdir [-p] dir...\n");
        return 1;
    }

    if (strcmp(argv[1], "-p") == 0) {
        create_parents = 1;
        i = 2;
    }

    if (i >= argc) {
        printf("usage: mkdir [-p] dir...\n");
        return 1;
    }

    for (; i < argc; i++) {
        if (ensure_dir(argv[i], create_parents) < 0) {
            printf("mkdir: cannot create %s, errno=%d\n",
                   argv[i],
                   errno);
            ret = 1;
        }
    }

    return ret;
}
