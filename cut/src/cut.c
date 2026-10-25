#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

#define MAX_RANGES 64

typedef struct {
    long lo, hi;
} range_t;

static range_t ranges[MAX_RANGES];
static int nranges;
static char delim = '\t';
static int only_delimited;

// 1,3-5,7- style lists
static int parse_list(const char *s) {
    while (*s) {
        if (nranges == MAX_RANGES)
            return -1;
        char *end;
        range_t r = { 1, 0x7fffffffL };

        if (*s != '-') {
            r.lo = strtol(s, &end, 10);
            if (end == s || r.lo < 1)
                return -1;
            s = end;
            r.hi = r.lo;
        }
        if (*s == '-') {
            s++;
            r.hi = 0x7fffffffL;
            if (*s >= '0' && *s <= '9') {
                r.hi = strtol(s, &end, 10);
                s = end;
            }
        }
        ranges[nranges++] = r;

        if (*s == ',')
            s++;
        else if (*s)
            return -1;
    }
    return nranges ? 0 : -1;
}

static int selected(long n) {
    for (int i = 0; i < nranges; i++) {
        if (n >= ranges[i].lo && n <= ranges[i].hi)
            return 1;
    }
    return 0;
}

static void cut_chars(const char *line) {
    long n = 1;
    for (const char *p = line; *p; p++, n++) {
        if (selected(n))
            putchar(*p);
    }
    putchar('\n');
}

static void cut_fields(const char *line) {
    if (!strchr(line, delim)) {
        if (!only_delimited)
            printf("%s\n", line);
        return;
    }

    long n = 1;
    int first = 1;
    const char *start = line;
    for (;;) {
        const char *end = strchr(start, delim);
        size_t len = end ? (size_t)(end - start) : strlen(start);
        if (selected(n)) {
            if (!first)
                putchar(delim);
            fwrite(start, 1, len, stdout);
            first = 0;
        }
        if (!end)
            break;
        start = end + 1;
        n++;
    }
    putchar('\n');
}

static void cut_file(FILE *f, int fields) {
    char *line = NULL;
    size_t len = 0;
    ssize_t r;
    while ((r = getline(&line, &len, f)) >= 0) {
        if (r > 0 && line[r - 1] == '\n')
            line[r - 1] = '\0';
        if (fields)
            cut_fields(line);
        else
            cut_chars(line);
    }
    free(line);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int fields = -1;
    const char *list = NULL;

    int c;
    while ((c = getopt(argc, argv, "f:c:b:d:s")) != -1) {
        switch (c) {
        case 'f': fields = 1; list = optarg; break;
        case 'c': case 'b': fields = 0; list = optarg; break;
        case 'd': delim = optarg[0]; break;
        case 's': only_delimited = 1; break;
        default:
            fprintf(stderr, "usage: cut -f list [-d delim] [-s] [file...]\n"
                            "       cut -c list [file...]\n");
            return 1;
        }
    }

    if (fields < 0) {
        warn("you must specify a list of bytes, characters, or fields");
        return 1;
    }
    if (parse_list(list) < 0) {
        warn("invalid list '%s'", list);
        return 1;
    }

    if (optind >= argc) {
        cut_file(stdin, fields);
        return 0;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        FILE *f = open_input(argv[i]);
        if (!f) {
            status = 1;
            continue;
        }
        cut_file(f, fields);
        close_input(f);
    }
    return status;
}
