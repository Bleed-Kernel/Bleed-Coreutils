#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

// how many digits after the point, so seq 1 0.5 3 prints 1.0 1.5 like gnu seq
static int decimals(const char *s) {
    const char *dot = strchr(s, '.');
    if (!dot)
        return 0;
    int n = 0;
    for (dot++; *dot >= '0' && *dot <= '9'; dot++)
        n++;
    return n;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *sep = "\n";
    const char *fmt = NULL;
    int equal_width = 0;

    // negative numbers look like options to getopt
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1] && !(argv[i][1] >= '0' && argv[i][1] <= '9') && argv[i][1] != '.') {
        if (!strcmp(argv[i], "-s") && i + 1 < argc) {
            sep = argv[++i];
        } else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fmt = argv[++i];
        } else if (!strcmp(argv[i], "-w")) {
            equal_width = 1;
        } else {
            fprintf(stderr, "usage: seq [-w] [-s sep] [-f format] [first [step]] last\n");
            return 1;
        }
        i++;
    }

    int nargs = argc - i;
    if (nargs < 1 || nargs > 3) {
        warn("missing operand");
        return 1;
    }

    double first = 1, step = 1, last;
    int prec = 0;
    if (nargs == 1) {
        last = strtod(argv[i], NULL);
    } else if (nargs == 2) {
        first = strtod(argv[i], NULL);
        last  = strtod(argv[i + 1], NULL);
        prec  = decimals(argv[i]);
    } else {
        first = strtod(argv[i], NULL);
        step  = strtod(argv[i + 1], NULL);
        last  = strtod(argv[i + 2], NULL);
        prec  = decimals(argv[i]);
        if (decimals(argv[i + 1]) > prec)
            prec = decimals(argv[i + 1]);
    }
    if (step == 0) {
        warn("invalid zero increment value");
        return 1;
    }

    int width = 0;
    if (equal_width) {
        char a[64], b[64];
        width = snprintf(a, sizeof(a), "%.*f", prec, first);
        int w = snprintf(b, sizeof(b), "%.*f", prec, last);
        if (w > width)
            width = w;
    }

    int printed = 0;
    for (long n = 0;; n++) {
        double v = first + step * (double)n;
        if ((step > 0 && v > last + 1e-9) || (step < 0 && v < last - 1e-9))
            break;
        if (printed)
            fputs(sep, stdout);
        if (fmt)
            printf(fmt, v);
        else if (equal_width)
            printf("%0*.*f", width, prec, v);
        else
            printf("%.*f", prec, v);
        printed = 1;
    }
    if (printed)
        putchar('\n');
    return 0;
}
