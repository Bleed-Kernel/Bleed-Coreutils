#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int parents;
static int verbose;

static int make(const char *path) {
    if (mkdir(path, 0755) == 0) {
        if (verbose)
            printf("%s: created directory '%s'\n", prog_name, path);
        return 0;
    }
    if (errno == EEXIST && parents && is_dir(path))
        return 0;
    return -1;
}

static int make_parents(const char *path) {
    char buf[PATH_MAX];
    if (strlen(path) >= sizeof(buf)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(buf, path);

    for (char *p = buf + 1; *p; p++) {
        if (*p != '/')
            continue;
        *p = '\0';
        if (!is_dir(buf) && make(buf) < 0)
            return -1;
        *p = '/';
        while (p[1] == '/')
            p++;
    }
    return make(buf);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "pvm:")) != -1) {
        switch (c) {
        case 'p':
            parents = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'm':
            // no permissions to set
            break;
        default:
            fprintf(stderr, "usage: mkdir [-pv] [-m mode] directory...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        int r = parents ? make_parents(argv[i]) : make(argv[i]);
        if (r < 0)
            status = warn("cannot create directory '%s': %s", argv[i], strerror(errno));
    }
    return status;
}
