#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    char buf[4096];
    size_t len = 0;

    if (argc < 2) {
        strcpy(buf, "y\n");
        len = 2;
    } else {
        for (int i = 1; i < argc; i++) {
            size_t n = strlen(argv[i]);
            if (len + n + 2 > sizeof(buf))
                break;
            memcpy(buf + len, argv[i], n);
            len += n;
            buf[len++] = i + 1 < argc ? ' ' : '\n';
        }
    }

    // fill the buffer up so each write does more than one line
    size_t one = len;
    while (len + one <= sizeof(buf)) {
        memcpy(buf + len, buf, one);
        len += one;
    }

    while (write_all(STDOUT_FILENO, buf, len) == 0)
        ;
    return 1;
}
