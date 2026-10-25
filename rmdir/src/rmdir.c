#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int parents;
static int verbose;
static int ignore_nonempty;

static int remove_dir(const char *path) {
    if (verbose)
        printf("%s: removing directory, '%s'\n", prog_name, path);
    if (rmdir(path) == 0)
        return 0;
    if (ignore_nonempty && errno == ENOTEMPTY)
        return 0;
    return warn("failed to remove '%s': %s", path, strerror(errno));
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    // --ignore-fail-on-non-empty is the only long option anyone uses
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ignore-fail-on-non-empty")) {
            ignore_nonempty = 1;
            for (int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argc--;
            i--;
        }
    }

    int c;
    while ((c = getopt(argc, argv, "pv")) != -1) {
        switch (c) {
        case 'p':
            parents = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            fprintf(stderr, "usage: rmdir [-pv] [--ignore-fail-on-non-empty] directory...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        if (remove_dir(argv[i])) {
            status = 1;
            continue;
        }
        if (!parents)
            continue;

        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%s", argv[i]);
        for (;;) {
            size_t len = strlen(buf);
            while (len > 1 && buf[len - 1] == '/')
                buf[--len] = '\0';
            char *slash = strrchr(buf, '/');
            if (!slash || slash == buf)
                break;
            *slash = '\0';
            if (remove_dir(buf)) {
                status = 1;
                break;
            }
        }
    }
    return status;
}
