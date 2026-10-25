#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <common.h>

static int show_count;
static int only_dups;
static int only_unique;
static int fold;

static void emit(const char *line, unsigned long n, FILE *out) {
    if (only_dups && n < 2)
        return;
    if (only_unique && n > 1)
        return;
    if (show_count)
        fprintf(out, "%7lu ", n);
    fputs(line, out);
    fputc('\n', out);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "cdui")) != -1) {
        switch (c) {
        case 'c': show_count = 1; break;
        case 'd': only_dups = 1; break;
        case 'u': only_unique = 1; break;
        case 'i': fold = 1; break;
        default:
            fprintf(stderr, "usage: uniq [-cdui] [input [output]]\n");
            return 1;
        }
    }

    FILE *in = stdin, *out = stdout;
    if (optind < argc && !(in = open_input(argv[optind])))
        return 1;
    if (optind + 1 < argc && !(out = fopen(argv[optind + 1], "w")))
        return warn("cannot create '%s'", argv[optind + 1]);

    char *prev = NULL, *line = NULL;
    size_t len = 0;
    unsigned long n = 0;
    ssize_t r;

    while ((r = getline(&line, &len, in)) >= 0) {
        if (r > 0 && line[r - 1] == '\n')
            line[r - 1] = '\0';

        if (prev && (fold ? strcasecmp(prev, line) : strcmp(prev, line)) == 0) {
            n++;
            continue;
        }
        if (prev) {
            emit(prev, n, out);
            free(prev);
        }
        prev = strdup(line);
        n = 1;
    }
    if (prev) {
        emit(prev, n, out);
        free(prev);
    }

    free(line);
    close_input(in);
    if (out != stdout)
        fclose(out);
    return 0;
}
