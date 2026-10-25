#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int canonical;
static long long skip;
static long long length = -1;

static void dump(FILE *f) {
    unsigned char line[16], prev[16];
    unsigned long long off = 0;
    int have_prev = 0, starred = 0;

    for (long long i = 0; i < skip && getc(f) != EOF; i++)
        off++;

    for (;;) {
        size_t want = sizeof(line);
        if (length >= 0 && (long long)want > length)
            want = (size_t)length;
        size_t n = want ? fread(line, 1, want, f) : 0;
        if (n == 0)
            break;
        if (length >= 0)
            length -= (long long)n;

        // repeated lines collapse into one * like the real thing
        if (have_prev && n == 16 && !memcmp(line, prev, 16)) {
            if (!starred)
                puts("*");
            starred = 1;
            off += n;
            continue;
        }
        starred = 0;

        if (canonical) {
            printf("%08llx  ", off);
            for (size_t i = 0; i < 16; i++) {
                if (i < n)
                    printf("%02x ", line[i]);
                else
                    printf("   ");
                if (i == 7)
                    putchar(' ');
            }
            printf(" |");
            for (size_t i = 0; i < n; i++)
                putchar(line[i] >= 32 && line[i] < 127 ? line[i] : '.');
            puts("|");
        } else {
            printf("%07llx", off);
            for (size_t i = 0; i < n; i += 2) {
                unsigned v = line[i] | (i + 1 < n ? line[i + 1] << 8 : 0);
                printf(" %04x", v);
            }
            putchar('\n');
        }

        memcpy(prev, line, n);
        have_prev = n == 16;
        off += n;
    }

    if (canonical)
        printf("%08llx\n", off);
    else
        printf("%07llx\n", off);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "Cs:n:v")) != -1) {
        switch (c) {
        case 'C': canonical = 1; break;
        case 's': skip = strtoll(optarg, NULL, 0); break;
        case 'n': length = strtoll(optarg, NULL, 0); break;
        case 'v': break;
        default:
            fprintf(stderr, "usage: hexdump [-C] [-s skip] [-n length] [file...]\n");
            return 1;
        }
    }

    if (optind >= argc) {
        dump(stdin);
        return 0;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        FILE *f = open_input(argv[i]);
        if (!f) {
            status = 1;
            continue;
        }
        dump(f);
        close_input(f);
    }
    return status;
}
