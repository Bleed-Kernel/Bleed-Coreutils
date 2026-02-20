#include <unistd.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    int i = 1;
    bool newline = true;

    if (i < argc && strcmp(argv[i], "-n") == 0) {
        newline = false;
        i++;
    }

    for (; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (len > 0)
            write(STDOUT_FILENO, argv[i], len);

        if (i + 1 < argc)
            write(STDOUT_FILENO, " ", 1);
    }

    if (newline)
        write(STDOUT_FILENO, "\n", 1);

    return 0;
}