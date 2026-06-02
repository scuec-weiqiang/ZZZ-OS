#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int test_pipe_basic(void)
{
    int p[2];
    pid_t pid;
    char buf[64];
    int status;

    if (pipe(p) < 0) {
        printf("pipe failed\n");
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        return -1;
    }

    if (pid == 0) {

        close(p[0]);

        write(p[1], "hello", 5);

        close(p[1]);

        exit(0);
    }

    close(p[1]);

    memset(buf, 0, sizeof(buf));

    read(p[0], buf, sizeof(buf));

    close(p[0]);

    waitpid(pid, &status, 0);

    if (strcmp(buf, "hello")) {
        printf("test_pipe_basic FAIL\n");
        return -1;
    }

    printf("test_pipe_basic PASS\n");

    return 0;
}

static int test_pipe_eof(void)
{
    int p[2];
    pid_t pid;
    int status;
    char buf[16];

    if (pipe(p) < 0)
        return -1;

    pid = fork();

    if (pid == 0) {

        close(p[0]);

        write(p[1], "A", 1);

        close(p[1]);

        exit(0);
    }

    close(p[1]);

    read(p[0], buf, 1);

    int n = read(p[0], buf, 1);

    waitpid(pid, &status, 0);

    close(p[0]);

    if (n != 0) {

        printf("test_pipe_eof FAIL n=%d\n", n);

        return -1;
    }

    printf("test_pipe_eof PASS\n");

    return 0;
}

static int test_dup2_stdout(void)
{
    int p[2];
    pid_t pid;
    char buf[64];
    int status;

    if (pipe(p) < 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return -1;

    if (pid == 0) {

        close(p[0]);

        if (dup2(p[1], STDOUT_FILENO) < 0) {
            exit(2);
        }

        close(p[1]);

        write(STDOUT_FILENO,
              "dup2-test",
              9);

        exit(0);
    }

    close(p[1]);

    memset(buf, 0, sizeof(buf));

    read(p[0], buf, sizeof(buf));

    waitpid(pid, &status, 0);

    close(p[0]);

    if (strcmp(buf, "dup2-test")) {

        printf("test_dup2_stdout FAIL got=%s\n",
               buf);

        return -1;
    }

    printf("test_dup2_stdout PASS\n");

    return 0;
}

static int test_large_transfer(void)
{
    int p[2];
    pid_t pid;
    int status;

    const int N = 8192;
    

    char *tx = malloc(N);
    char *rx = malloc(N);
    printf("tx=%p rx=%p\n", tx, rx);

    // memset(tx, 0x55, 8192);
    // memset(rx, 0xaa, 8192);

    printf("memset ok\n");

    int total;

    if (!tx || !rx)
        return -1;

    for (int i = 0; i < N; i++)
        tx[i] = i & 0xff;

    if (pipe(p) < 0)
        return -1;

    pid = fork();

    if (pid == 0) {

        close(p[0]);

        total = 0;

        while (total < N) {

            int n =
                write(p[1],
                      tx + total,
                      N - total);

            if (n <= 0)
                exit(2);

            total += n;
        }

        close(p[1]);

        exit(0);
    }

    close(p[1]);

    total = 0;

    while (total < N) {

        int n =
            read(p[0],
                 rx + total,
                 N - total);

        if (n <= 0)
            break;

        total += n;
    }

    waitpid(pid, &status, 0);

    close(p[0]);

    if (total != N) {

        printf("test_large_transfer FAIL size=%d\n",
               total);

        return -1;
    }

    if (memcmp(tx, rx, N)) {

        printf("test_large_transfer FAIL data mismatch\n");

        return -1;
    }

    free(tx);
    free(rx);

    printf("test_large_transfer PASS\n");

    return 0;
}

int main(void)
{
    if (test_pipe_basic())
        return 1;

    if (test_pipe_eof())
        return 1;

    if (test_dup2_stdout())
        return 1;

    if (test_large_transfer())
        return 1;

    printf("\nALL PIPE TESTS PASS\n");

    return 0;
}