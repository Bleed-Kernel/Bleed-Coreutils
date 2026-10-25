#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <common.h>

static int reverse;
static int numeric;
static int fold;
static int unique;
static int key_field;
static char separator;

static char **lines;
static size_t nlines, cap;

static void add_lines(FILE *f) {
    char *line = NULL;
    size_t len = 0;
    ssize_t r;

    while ((r = getline(&line, &len, f)) >= 0) {
        if (r > 0 && line[r - 1] == '\n')
            line[r - 1] = '\0';
        if (nlines == cap) {
            cap = cap ? cap * 2 : 256;
            lines = realloc(lines, cap * sizeof(char *));
        }
        lines[nlines++] = strdup(line);
    }
    free(line);
}

// start of field n (1 based), blank runs split fields unless -t was given
static const char *field(const char *s, int n) {
    for (int i = 1; i < n; i++) {
        if (separator) {
            const char *p = strchr(s, separator);
            if (!p)
                return s + strlen(s);
            s = p + 1;
        } else {
            while (*s && isblank((unsigned char)*s))
                s++;
            while (*s && !isblank((unsigned char)*s))
                s++;
        }
    }
    return s;
}

static int line_cmp(const char *a, const char *b) {
    if (key_field) {
        a = field(a, key_field);
        b = field(b, key_field);
    }

    int r;
    if (numeric) {
        double x = strtod(a, NULL), y = strtod(b, NULL);
        r = x < y ? -1 : x > y ? 1 : 0;
    } else if (fold) {
        r = strcasecmp(a, b);
    } else {
        r = strcmp(a, b);
    }
    return r;
}

static int qsort_cmp(const void *a, const void *b) {
    int r = line_cmp(*(char *const *)a, *(char *const *)b);
    if (r == 0 && !unique)
        r = strcmp(*(char *const *)a, *(char *const *)b);
    return reverse ? -r : r;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *output = NULL;

    int c;
    while ((c = getopt(argc, argv, "rnfuk:t:o:bg")) != -1) {
        switch (c) {
        case 'r': reverse = 1; break;
        case 'n': case 'g': numeric = 1; break;
        case 'f': fold = 1; break;
        case 'u': unique = 1; break;
        case 'k': key_field = atoi(optarg); break;
        case 't': separator = optarg[0]; break;
        case 'o': output = optarg; break;
        case 'b': break;
        default:
            fprintf(stderr, "usage: sort [-rnfub] [-k field] [-t sep] [-o file] [file...]\n");
            return 2;
        }
    }

    int status = 0;
    if (optind >= argc) {
        add_lines(stdin);
    } else {
        for (int i = optind; i < argc; i++) {
            FILE *f = open_input(argv[i]);
            if (!f) {
                status = 2;
                continue;
            }
            add_lines(f);
            close_input(f);
        }
    }

    qsort(lines, nlines, sizeof(char *), qsort_cmp);

    // after reading so sort -o file file works
    FILE *out = stdout;
    if (output && !(out = fopen(output, "w")))
        return warn("cannot create '%s'", output) + 1;

    for (size_t i = 0; i < nlines; i++) {
        if (unique && i > 0 && line_cmp(lines[i - 1], lines[i]) == 0)
            continue;
        fputs(lines[i], out);
        fputc('\n', out);
    }

    if (out != stdout)
        fclose(out);
    return status;
}
