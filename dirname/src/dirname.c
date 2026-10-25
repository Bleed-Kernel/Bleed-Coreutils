#include <stdio.h>
#include <string.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);

    if (argc < 2) {
        warn("missing operand");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char buf[4096];
        snprintf(buf, sizeof(buf), "%s", argv[i]);

        size_t len = strlen(buf);
        while (len > 1 && buf[len - 1] == '/')
            len--;
        while (len > 0 && buf[len - 1] != '/')
            len--;
        while (len > 1 && buf[len - 1] == '/')
            len--;

        if (len == 0)
            strcpy(buf, ".");
        else
            buf[len] = '\0';
        puts(buf);
    }
    return 0;
}
