#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int argi = 1;
    int newline = 1;

    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = 0;
        argi = 2;
    }

    for (; argi < argc; argi++) {
        if (argi > ((newline == 0) ? 2 : 1)) {
            printf(" ");
        }
        printf("%s", argv[argi]);
    }

    if (newline) {
        printf("\n");
    }

    return 0;
}
