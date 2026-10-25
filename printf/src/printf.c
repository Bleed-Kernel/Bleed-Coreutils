#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>

static char **args;
static int nargs;
static int used;
static int status;

static const char *next_arg(void) {
    return used < nargs ? args[used++] : NULL;
}

static long long arg_int(void) {
    const char *a = next_arg();
    if (!a)
        return 0;
    // 'c gives the character code
    if (a[0] == '\'' || a[0] == '"')
        return (unsigned char)a[1];
    char *end;
    long long v = strtoll(a, &end, 0);
    if (*end) {
        warn("'%s': expected a numeric value", a);
        status = 1;
    }
    return v;
}

static double arg_float(void) {
    const char *a = next_arg();
    return a ? strtod(a, NULL) : 0.0;
}

// handles one backslash escape, returns how many bytes of s it ate. stop gets set by \c
static int escape(const char *s, int octal_zero, int *stop) {
    int v, n = 1;
    switch (s[0]) {
    case 'n': putchar('\n'); return 1;
    case 't': putchar('\t'); return 1;
    case 'r': putchar('\r'); return 1;
    case 'a': putchar('\a'); return 1;
    case 'b': putchar('\b'); return 1;
    case 'f': putchar('\f'); return 1;
    case 'v': putchar('\v'); return 1;
    case 'e': putchar(0x1b); return 1;
    case '\\': putchar('\\'); return 1;
    case 'c':
        if (stop)
            *stop = 1;
        return 1;
    case 'x':
        v = 0;
        while (n < 3 && strchr("0123456789abcdefABCDEF", s[n]) && s[n]) {
            v = v * 16 + (s[n] <= '9' ? s[n] - '0' : (s[n] | 0x20) - 'a' + 10);
            n++;
        }
        putchar(v);
        return n;
    default:
        break;
    }

    // \NNN in the format, \0NNN in %b
    if (s[0] >= '0' && s[0] <= '7') {
        int i = 0, max = 3;
        v = 0;
        if (octal_zero && s[0] == '0') {
            i = 1;
            max = 4;
        }
        while (i < max && s[i] >= '0' && s[i] <= '7')
            v = v * 8 + (s[i++] - '0');
        putchar(v);
        return i;
    }

    putchar('\\');
    putchar(s[0]);
    return 1;
}

// one pass over the format, returns 1 if \c said to stop everything
static int run_format(const char *fmt) {
    for (const char *p = fmt; *p; p++) {
        if (*p == '\\' && p[1]) {
            int stop = 0;
            p += escape(p + 1, 0, &stop);
            if (stop)
                return 1;
            continue;
        }
        if (*p != '%') {
            putchar(*p);
            continue;
        }
        if (p[1] == '%') {
            putchar('%');
            p++;
            continue;
        }

        char spec[64];
        int n = 0;
        spec[n++] = '%';
        p++;
        while (*p && strchr("-+ #0", *p) && n < 40)
            spec[n++] = *p++;
        if (*p == '*') {
            n += snprintf(spec + n, sizeof(spec) - n, "%d", (int)arg_int());
            p++;
        }
        while (*p >= '0' && *p <= '9' && n < 50)
            spec[n++] = *p++;
        if (*p == '.') {
            spec[n++] = *p++;
            if (*p == '*') {
                n += snprintf(spec + n, sizeof(spec) - n, "%d", (int)arg_int());
                p++;
            }
            while (*p >= '0' && *p <= '9' && n < 58)
                spec[n++] = *p++;
        }
        while (*p && strchr("hlLqjzt", *p))
            p++;
        if (!*p)
            break;

        char conv = *p;
        switch (conv) {
        case 'd': case 'i':
            spec[n++] = 'l'; spec[n++] = 'l'; spec[n++] = conv; spec[n] = '\0';
            printf(spec, arg_int());
            break;
        case 'u': case 'x': case 'X': case 'o':
            spec[n++] = 'l'; spec[n++] = 'l'; spec[n++] = conv; spec[n] = '\0';
            printf(spec, (unsigned long long)arg_int());
            break;
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G':
            spec[n++] = conv; spec[n] = '\0';
            printf(spec, arg_float());
            break;
        case 'c': {
            const char *a = next_arg();
            spec[n++] = 'c'; spec[n] = '\0';
            printf(spec, a ? a[0] : '\0');
            break;
        }
        case 's': {
            const char *a = next_arg();
            spec[n++] = 's'; spec[n] = '\0';
            printf(spec, a ? a : "");
            break;
        }
        case 'b': {
            const char *a = next_arg();
            for (const char *q = a ? a : ""; *q; q++) {
                if (*q == '\\' && q[1]) {
                    int stop = 0;
                    q += escape(q + 1, 1, &stop);
                    if (stop)
                        return 1;
                } else {
                    putchar(*q);
                }
            }
            break;
        }
        default:
            warn("%%%c: invalid conversion specification", conv);
            status = 1;
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int first = 1;
    if (argc > 1 && !strcmp(argv[1], "--"))
        first = 2;
    if (argc <= first) {
        warn("missing operand");
        return 1;
    }

    const char *fmt = argv[first];
    args  = argv + first + 1;
    nargs = argc - first - 1;

    // the format gets reused until every argument is eaten
    do {
        int before = used;
        if (run_format(fmt))
            break;
        if (used == before)
            break;
    } while (used < nargs);

    return status;
}
