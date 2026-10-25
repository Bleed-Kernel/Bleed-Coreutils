#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);

    if (argc < 2) {
        warn("missing operand");
        return 1;
    }

    double total = 0;
    for (int i = 1; i < argc; i++) {
        char *end;
        double v = strtod(argv[i], &end);
        switch (*end) {
        case '\0': case 's': break;
        case 'm': v *= 60; break;
        case 'h': v *= 3600; break;
        case 'd': v *= 86400; break;
        default:
            warn("invalid time interval '%s'", argv[i]);
            return 1;
        }
        total += v;
    }

    struct timespec ts;
    ts.tv_sec  = (time_t)total;
    ts.tv_nsec = (long)((total - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
    return 0;
}
