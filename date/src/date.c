#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *fmt = "%a %b %e %H:%M:%S %Z %Y";
    time_t t = time(NULL);

    int c;
    while ((c = getopt(argc, argv, "uRId:r:")) != -1) {
        switch (c) {
        case 'u':
            // theres only utc anyway
            break;
        case 'R':
            fmt = "%a, %d %b %Y %H:%M:%S %z";
            break;
        case 'I':
            fmt = "%Y-%m-%d";
            break;
        case 'd':
            // only @seconds, no date parser
            if (optarg[0] != '@')
                return warn("invalid date '%s', only @seconds is supported", optarg);
            t = (time_t)strtoll(optarg + 1, NULL, 10);
            break;
        case 'r': {
            struct stat st;
            if (stat(optarg, &st) < 0)
                return warn("%s: cannot stat", optarg);
            t = st.st_mtime;
            break;
        }
        default:
            fprintf(stderr, "usage: date [-uRI] [-d @seconds] [-r file] [+format]\n");
            return 1;
        }
    }

    if (optind < argc) {
        if (argv[optind][0] != '+')
            return warn("setting the date isnt supported");
        fmt = argv[optind] + 1;
    }

    char buf[512];
    struct tm *tm = gmtime(&t);
    if (!tm)
        return warn("invalid time");
    strftime(buf, sizeof(buf), fmt, tm);
    puts(buf);
    return 0;
}
