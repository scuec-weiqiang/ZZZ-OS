#include <../user/printf.h>

int main () {
    int a = 0;
    while (1) {
        for (int i = 0; i < 10000000; i++) {
        }
        printf("proc1 :Hello, World! %d\r",a);
        a++;
        if (a == 1000000000) {
            a = 0;
        }
    }
    return 0;
}