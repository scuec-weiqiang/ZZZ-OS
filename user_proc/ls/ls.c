#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern int _getdents(int fd, void *dirp, unsigned int count);

struct linux_dirent {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[0];
};

#define CLR_RESET  "\033[0m"

#define CLR_DIR    "\033[34m"
#define CLR_EXEC   "\033[32m"
#define CLR_DEV    "\033[33m"

int main(int argc, char *argv[]) {
    char buf[512];

    const char *path = ".";

    int fd;
    int nread;

    int col = 0;

    if (argv[1])
        path = argv[1];

    fd = open(path, O_RDONLY, 0);

    if (fd < 0) {

        printf("ls: cannot open %s errno=%d\n",
               path,
               errno);

        return -1;
    }

    for (;;) {

        int bpos = 0;

        nread = _getdents(fd,
                          buf,
                          sizeof(buf));

        if (nread < 0) {

            printf("ls: getdents failed errno=%d\n",
                   errno);

            close(fd);

            return -1;
        }

        if (nread == 0)
            break;

        while (bpos < nread) {

            struct linux_dirent *d =
                (struct linux_dirent *)(buf + bpos);

            char fullpath[256];

            struct stat st;

            if (d->d_name[0] == '\0')
                goto next;

            /*
             * 隐藏 . ..
             */
            if (!strcmp(d->d_name, ".") ||
                !strcmp(d->d_name, ".."))
                goto next;

            /*
             * path/name
             */
            snprintf(fullpath,
                     sizeof(fullpath),
                     "%s/%s",
                     path,
                     d->d_name);

            if (stat(fullpath, &st) < 0) {

                printf("%-16s",
                       d->d_name);

                goto next_print;
            }

            /*
             * directory
             */
            if (S_ISDIR(st.st_mode)) {
                char dir_name[64];
                // 先拼接成 "dev/"
                snprintf(dir_name, sizeof(dir_name), "%s/", d->d_name);

                printf(CLR_DIR
                       "%-15s"
                       CLR_RESET
                       " ",
                       dir_name);
            }

            /*
             * executable
             */
            else if (st.st_mode & 0111) {

                printf(CLR_EXEC
                       "%-16s"
                       CLR_RESET,
                       d->d_name);
            }

            /*
             * regular file
             */
            else {

                printf("%-16s",
                       d->d_name);
            }

next_print:

            col++;

            if (col % 4 == 0)
                printf("\n");

next:
            if (d->d_reclen == 0)
                break;

            bpos += d->d_reclen;
        }
    }

    if (col % 4 != 0)
        printf("\n");

    close(fd);

    return 0;
}