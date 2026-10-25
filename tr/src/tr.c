#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

// expands escapes, a-z ranges and [:classes:] into a flat list of bytes
static int expand(const char *s, unsigned char *out) {
    int n = 0;
    while (*s && n < 256) {
        if (s[0] == '[' && s[1] == ':') {
            const char *end = strstr(s, ":]");
            if (end) {
                char cls[16] = {0};
                size_t len = (size_t)(end - s - 2);
                if (len < sizeof(cls))
                    memcpy(cls, s + 2, len);
                for (int ch = 0; ch < 256 && n < 256; ch++) {
                    int in = (!strcmp(cls, "lower") && islower(ch)) || (!strcmp(cls, "upper") && isupper(ch)) ||
                             (!strcmp(cls, "digit") && isdigit(ch)) || (!strcmp(cls, "alpha") && isalpha(ch)) ||
                             (!strcmp(cls, "alnum") && isalnum(ch)) || (!strcmp(cls, "space") && isspace(ch)) ||
                             (!strcmp(cls, "blank") && isblank(ch)) || (!strcmp(cls, "punct") && ispunct(ch)) ||
                             (!strcmp(cls, "cntrl") && iscntrl(ch)) || (!strcmp(cls, "print") && isprint(ch)) ||
                             (!strcmp(cls, "xdigit") && isxdigit(ch));
                    if (in)
                        out[n++] = (unsigned char)ch;
                }
                s = end + 2;
                continue;
            }
        }

        unsigned char ch = (unsigned char)*s++;
        if (ch == '\\' && *s) {
            switch (*s++) {
            case 'n':  ch = '\n'; break;
            case 't':  ch = '\t'; break;
            case 'r':  ch = '\r'; break;
            case '\\': ch = '\\'; break;
            case '0':  ch = '\0'; break;
            default:   ch = (unsigned char)s[-1]; break;
            }
        }

        if (s[0] == '-' && s[1]) {
            unsigned char hi = (unsigned char)s[1];
            s += 2;
            for (int c = ch; c <= hi && n < 256; c++)
                out[n++] = (unsigned char)c;
            continue;
        }
        out[n++] = ch;
    }
    return n;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int delete = 0, squeeze = 0, complement = 0;

    int c;
    while ((c = getopt(argc, argv, "dscC")) != -1) {
        switch (c) {
        case 'd': delete = 1; break;
        case 's': squeeze = 1; break;
        case 'c': case 'C': complement = 1; break;
        default:
            fprintf(stderr, "usage: tr [-cds] set1 [set2]\n");
            return 1;
        }
    }

    int nargs = argc - optind;
    if (nargs < 1 || (!delete && !squeeze && nargs < 2)) {
        warn("missing operand");
        return 1;
    }

    unsigned char set1[256], set2[256];
    int n1 = expand(argv[optind], set1);
    int n2 = nargs > 1 ? expand(argv[optind + 1], set2) : 0;

    unsigned char in1[256] = {0};
    for (int i = 0; i < n1; i++)
        in1[set1[i]] = 1;
    if (complement) {
        for (int i = 0; i < 256; i++)
            in1[i] = !in1[i];
    }

    int map[256];
    for (int i = 0; i < 256; i++)
        map[i] = i;
    if (!delete && n2 > 0) {
        if (complement) {
            for (int i = 0; i < 256; i++) {
                if (in1[i])
                    map[i] = set2[n2 - 1];
            }
        } else {
            // a short set2 repeats its last byte like gnu tr
            for (int i = 0; i < n1; i++)
                map[set1[i]] = set2[i < n2 ? i : n2 - 1];
        }
    }

    // -s squeezes on set2 when translating, set1 otherwise
    unsigned char squeeze_set[256] = {0};
    if (squeeze) {
        if (n2 > 0 && !delete) {
            for (int i = 0; i < n2; i++)
                squeeze_set[set2[i]] = 1;
        } else if (delete && n2 > 0) {
            for (int i = 0; i < n2; i++)
                squeeze_set[set2[i]] = 1;
        } else {
            memcpy(squeeze_set, in1, sizeof(squeeze_set));
        }
    }

    int ch, last = -1;
    while ((ch = getchar()) != EOF) {
        if (delete && in1[ch])
            continue;
        int o = map[ch];
        if (squeeze && o == last && squeeze_set[o])
            continue;
        putchar(o);
        last = o;
    }
    return 0;
}
