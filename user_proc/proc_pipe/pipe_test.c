#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern int pipe(int pipefd[2]);

#define TEST_PIPE_BUF 4096

static int make_pipe(int fds[2]) {
    if (pipe(fds) < 0) {
        perror("pipe");
        return -1;
    }
    return 0;
}

static int test_basic_transfer(void) {
    int fds[2];
    pid_t pid;
    int status = 0;
    const char *msg = "pipe-hello";
    char buf[32];
    ssize_t n;

    if (make_pipe(fds) < 0)
        return -1;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        close(fds[1]);
        memset(buf, 0, sizeof(buf));
        n = read(fds[0], buf, sizeof(buf) - 1);
        close(fds[0]);
        if (n < 0) {
            perror("child read");
            _exit(2);
        }
        if (strcmp(buf, msg) != 0) {
            printf("basic mismatch: got=\"%s\" expect=\"%s\"\n", buf, msg);
            _exit(3);
        }
        _exit(0);
    }

    close(fds[0]);
    n = write(fds[1], msg, strlen(msg) + 1);
    close(fds[1]);
    if (n < 0) {
        perror("parent write");
        return -1;
    }

    if (wait(&status) < 0) {
        perror("wait");
        return -1;
    }
    if (status != 0) {
        printf("test_basic_transfer FAIL: child exit=%d\n", status);
        return -1;
    }

    printf("test_basic_transfer PASS\n");
    return 0;
}

/*
 * 读阻塞场景：
 * child 先发 ready，再执行 read(data_rd)；
 * parent 收到 ready 后才写入数据，child 才能继续。
 */
static int test_read_block_path(void) {
    int data[2];
    int sync[2];
    pid_t pid;
    int status = 0;
    char ch = 0;
    char out = 'X';
    char in = 0;

    if (make_pipe(data) < 0)
        return -1;
    if (make_pipe(sync) < 0)
        return -1;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        close(data[1]);
        close(sync[0]);

        ch = 'R';
        if (write(sync[1], &ch, 1) != 1)
            _exit(2);

        if (read(data[0], &in, 1) != 1)
            _exit(3);
        if (in != out)
            _exit(4);

        ch = 'D';
        if (write(sync[1], &ch, 1) != 1)
            _exit(5);

        close(data[0]);
        close(sync[1]);
        _exit(0);
    }

    close(data[0]);
    close(sync[1]);

    if (read(sync[0], &ch, 1) != 1 || ch != 'R') {
        printf("test_read_block_path FAIL: ready handshake failed\n");
        return -1;
    }

    if (write(data[1], &out, 1) != 1) {
        perror("parent write");
        return -1;
    }

    if (read(sync[0], &ch, 1) != 1 || ch != 'D') {
        printf("test_read_block_path FAIL: done handshake failed\n");
        return -1;
    }

    close(data[1]);
    close(sync[0]);

    if (wait(&status) < 0) {
        perror("wait");
        return -1;
    }
    if (status != 0) {
        printf("test_read_block_path FAIL: child exit=%d\n", status);
        return -1;
    }

    printf("test_read_block_path PASS\n");
    return 0;
}

/*
 * 写阻塞路径：
 * parent 先把管道写满，再写 1 字节；
 * child 延迟后关闭读端，parent 应从阻塞中返回并得到 EPIPE。
 */
static int test_write_block_until_epipe(void) {
    int data[2];
    int sync[2];
    pid_t pid;
    int status = 0;
    static char bigbuf[TEST_PIPE_BUF];
    char ch = 0;
    ssize_t n;
    volatile int delay;

    memset(bigbuf, 'A', sizeof(bigbuf));

    if (make_pipe(data) < 0)
        return -1;
    if (make_pipe(sync) < 0)
        return -1;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        close(data[1]);
        close(sync[0]);

        ch = 'R';
        if (write(sync[1], &ch, 1) != 1)
            _exit(2);

        for (delay = 0; delay < 2000000; delay++) {
        }

        close(data[0]);
        close(sync[1]);
        _exit(0);
    }

    close(data[0]);
    close(sync[1]);

    if (read(sync[0], &ch, 1) != 1 || ch != 'R') {
        printf("test_write_block_until_epipe FAIL: ready handshake failed\n");
        return -1;
    }

    n = write(data[1], bigbuf, sizeof(bigbuf));
    if (n != (ssize_t)sizeof(bigbuf)) {
        printf("test_write_block_until_epipe FAIL: fill write=%ld\n", (long)n);
        return -1;
    }

    errno = 0;
    n = write(data[1], "Z", 1);
    if (n >= 0 || errno != EPIPE) {
        printf("test_write_block_until_epipe FAIL: n=%ld errno=%d\n", (long)n, errno);
        return -1;
    }

    close(data[1]);
    close(sync[0]);

    if (wait(&status) < 0) {
        perror("wait");
        return -1;
    }
    if (status != 0) {
        printf("test_write_block_until_epipe FAIL: child exit=%d\n", status);
        return -1;
    }

    printf("test_write_block_until_epipe PASS\n");
    return 0;
}

int main(void) {
    if (test_basic_transfer() < 0)
        return 1;
    if (test_read_block_path() < 0)
        return 1;
    if (test_write_block_until_epipe() < 0)
        return 1;

    printf("pipe test ALL PASS\n");
    return 0;
}
