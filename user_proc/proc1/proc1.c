#include <../user/printf.h>
#include <../user/usys.h>

int main (int argc, char *argv[]) {
    printf("init :Hello, World! pid = %x\r",getpid());
    exit(0);
}