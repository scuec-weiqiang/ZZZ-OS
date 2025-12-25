#include <../user/printf.h>

int main () {
    int a = 0;
    while (1) {
        for (int i = 0; i < 100000000; i++) {
        }
        printf("proc2 :Hello, World! %d\r",a);
        a++;
        if (a == 10) {
            a = 0;
        }
    }
    return 0;
}