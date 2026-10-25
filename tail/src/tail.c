#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static long long count = 10;
static int bytes_mode;
static int from_start;

// slurps the whole thing, files on here are small and pipes cant seek anyway
static char *read_all(FILE *f, size_t *len) {
    size_t cap = 65536, n = 0;
    char *buf = malloc(cap);
    if (!buf)
        return NULL;

    size_t r;
    while ((r = fread(buf + n, 1, cap - n, f)) > 0) {
        n += r;
        if (n == cap) {
            cap *= 2;
            char *grown = realloc(buf, cap);
            if (!grown)
                break;
            buf = grown;
        }
    }
    *len = n;
    return buf;
}

static void tail(FILE *f) {
    size_t len = 0;
    char *buf = read_all(f, &len);
    if (!buf)
        return;

    size_t start = 0;
    if (bytes_mode) {
        if (from_start)
            start = count > 0 ? (size_t)(count - 1) : 0;
        else
            start = (size_t)count < len ? len - (size_t)count : 0;
    } else if (from_start) {
        long long line = 1;
        while (start < len && line < count) {
            if (buf[start++] == '\n')
                line++;
        }
    } else {
        // a trailing newline doesnt start another line
        size_t i = len;
        if (i > 0 && buf[i - 1] == '\n')
            i--;
        long long lines = 0;
        while (i > 0) {
            if (buf[i - 1] == '\n' && ++lines == count)
                break;
            i--;
        }
        start = count == 0 ? len : i;
    }

    if (start > len)
        start = len;
    fwrite(buf + start, 1, len - start, stdout);
    free(buf);
}

static void parse_count(const char *s) {
    from_start = s[0] == '+';
    count = atoll(s[0] == '+' || s[0] == '-' ? s + 1 : s);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int quiet = 0, verbose = 0;

    if (argc > 1 && argv[1][0] == '-' && isdigit((unsigned char)argv[1][1])) {
        count = atoll(argv[1] + 1);
        argv[1] = argv[0];
        argv++;
        argc--;
    }

    int c;
    while ((c = getopt(argc, argv, "n:c:qvf")) != -1) {
        switch (c) {
        case 'n':
            parse_count(optarg);
            bytes_mode = 0;
            break;
        case 'c':
            parse_count(optarg);
            bytes_mode = 1;
            break;
        case 'q': quiet = 1; break;
        case 'v': verbose = 1; break;
        case 'f': break;
        default:
            fprintf(stderr, "usage: tail [-n [+]lines | -c [+]bytes] [file...]\n");
            return 1;
        }
    }

    if (optind >= argc) {
        tail(stdin);
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
        tail(f);
        close_input(f);
    }
    return status;
}
