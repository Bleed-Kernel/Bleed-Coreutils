#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static long long count = 10;
static int bytes_mode;

static void head(FILE *f) {
    int ch;
    long long n = 0;

    if (bytes_mode) {
        while (n < count && (ch = getc(f)) != EOF) {
            putchar(ch);
            n++;
        }
        return;
    }

    while (n < count && (ch = getc(f)) != EOF) {
        putchar(ch);
        if (ch == '\n')
            n++;
    }
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int quiet = 0, verbose = 0;

    // head -5 is the old way of saying head -n 5 and everyone still types it
    if (argc > 1 && argv[1][0] == '-' && isdigit((unsigned char)argv[1][1])) {
        count = atoll(argv[1] + 1);
        argv[1] = argv[0];
        argv++;
        argc--;
    }

    int c;
    while ((c = getopt(argc, argv, "n:c:qv")) != -1) {
        switch (c) {
        case 'n':
            count = atoll(optarg);
            bytes_mode = 0;
            break;
        case 'c':
            count = atoll(optarg);
            bytes_mode = 1;
            break;
        case 'q': quiet = 1; break;
        case 'v': verbose = 1; break;
        default:
            fprintf(stderr, "usage: head [-n lines | -c bytes] [file...]\n");
            return 1;
        }
    }

    if (optind >= argc) {
        head(stdin);
        return 0;
    }

    int status = 0;
    int headers = (argc - optind > 1 || verbose) && !quiet;
    for (int i = optind; i < argc; i++) {
        FILE *f = open_input(argv[i]);
        if (!f) {
            status = 1;
            continue;
        }
        if (headers)
            printf("%s==> %s <==\n", i == optind ? "" : "\n", argv[i]);
        head(f);
        close_input(f);
    }
    return status;
}
