#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    char buf[128];

    printf("before fgets\n");
    fflush(stdout);

    char *p = fgets(buf, sizeof(buf), stdin);

    printf("after fgets p=%p errno=%d buf=[%s]\n", p, errno, p ? buf : "");
    return 0;
}
