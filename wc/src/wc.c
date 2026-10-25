#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int show_lines, show_words, show_bytes, show_chars, show_longest;

typedef struct {
    unsigned long long lines, words, bytes, chars, longest;
} counts_t;

static void count(FILE *f, counts_t *c) {
    int ch, in_word = 0;
    unsigned long long line_len = 0;

    while ((ch = getc(f)) != EOF) {
        c->bytes++;
        // utf-8 continuation bytes arent new characters
        if ((ch & 0xC0) != 0x80)
            c->chars++;

        if (ch == '\n') {
            c->lines++;
            if (line_len > c->longest)
                c->longest = line_len;
            line_len = 0;
        } else {
            line_len++;
        }

        if (isspace(ch)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            c->words++;
        }
    }
    if (line_len > c->longest)
        c->longest = line_len;
}

static void print(const counts_t *c, const char *name) {
    int width = 7;
    if (show_lines)   printf("%*llu ", width, c->lines);
    if (show_words)   printf("%*llu ", width, c->words);
    if (show_chars)   printf("%*llu ", width, c->chars);
    if (show_bytes)   printf("%*llu ", width, c->bytes);
    if (show_longest) printf("%*llu ", width, c->longest);
    if (name)
        printf("%s", name);
    putchar('\n');
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "lwcmL")) != -1) {
        switch (c) {
        case 'l': show_lines = 1; break;
        case 'w': show_words = 1; break;
        case 'c': show_bytes = 1; break;
        case 'm': show_chars = 1; break;
        case 'L': show_longest = 1; break;
        default:
            fprintf(stderr, "usage: wc [-lwcmL] [file...]\n");
            return 1;
        }
    }
    if (!show_lines && !show_words && !show_bytes && !show_chars && !show_longest)
        show_lines = show_words = show_bytes = 1;

    if (optind >= argc) {
        counts_t cnt = {0};
        count(stdin, &cnt);
        print(&cnt, NULL);
        return 0;
    }

    int status = 0;
    counts_t total = {0};
    for (int i = optind; i < argc; i++) {
        FILE *f = open_input(argv[i]);
        if (!f) {
            status = 1;
            continue;
        }
        counts_t cnt = {0};
        count(f, &cnt);
        close_input(f);
        print(&cnt, argv[i]);

        total.lines += cnt.lines;
        total.words += cnt.words;
        total.bytes += cnt.bytes;
        total.chars += cnt.chars;
        if (cnt.longest > total.longest)
            total.longest = cnt.longest;
    }
    if (argc - optind > 1)
        print(&total, "total");
    return status;
}
