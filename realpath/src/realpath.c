#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int quiet = 0;

    int c;
    while ((c = getopt(argc, argv, "qesmL")) != -1) {
        switch (c) {
        case 'q': quiet = 1; break;
        case 'e': case 's': case 'm': case 'L': break;
        default:
            fprintf(stderr, "usage: realpath [-q] file...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        char buf[PATH_MAX];
        if (!realpath(argv[i], buf)) {
            status = 1;
            if (!quiet)
                warn("%s: %s", argv[i], strerror(errno));
            continue;
        }
        puts(buf);
    }
    return status;
}
