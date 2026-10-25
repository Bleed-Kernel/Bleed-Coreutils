#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int silent = 0, list = 0;

    int c;
    while ((c = getopt(argc, argv, "sl")) != -1) {
        switch (c) {
        case 's': silent = 1; break;
        case 'l': list = 1; break;
        default:
            fprintf(stderr, "usage: cmp [-sl] file1 [file2]\n");
            return 2;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 2;
    }

    const char *n1 = argv[optind];
    const char *n2 = optind + 1 < argc ? argv[optind + 1] : "-";
    FILE *a = open_input(n1);
    FILE *b = a ? open_input(n2) : NULL;
    if (!a || !b) {
        close_input(a);
        return 2;
    }

    unsigned long long byte = 1, line = 1;
    int status = 0;
    for (;;) {
        int x = getc(a), y = getc(b);
        if (x == EOF && y == EOF)
            break;
        if (x == EOF || y == EOF) {
            if (!silent)
                fprintf(stderr, "cmp: EOF on %s after byte %llu\n", x == EOF ? n1 : n2, byte - 1);
            status = 1;
            break;
        }
        if (x != y) {
            status = 1;
            if (silent)
                break;
            if (!list) {
                printf("%s %s differ: byte %llu, line %llu\n", n1, n2, byte, line);
                break;
            }
            printf("%llu %3o %3o\n", byte, x, y);
        }
        if (x == '\n')
            line++;
        byte++;
    }

    close_input(a);
    close_input(b);
    return status;
}
