#include <stdio.h>

extern char **environ;

int main(void)
{
    char **envp = environ;

    while (envp != NULL && *envp != NULL) {
        printf("%s\n", *envp);
        envp++;
    }

    return 0;
}
