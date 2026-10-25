#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int recursive;
static int force;
static int interactive;
static int dirs;
static int verbose;

static int remove_one(const char *path) {
    const char *base = path_base(path);
    if (!strcmp(base, ".") || !strcmp(base, "..") || !strcmp(base, "./") || !strcmp(base, "../"))
        return warn("refusing to remove '.' or '..' directory: skipping '%s'", path);

    struct stat st;
    if (stat(path, &st) < 0) {
        if (force && errno == ENOENT)
            return 0;
        return warn("cannot remove '%s': %s", path, strerror(errno));
    }

    if (S_ISDIR(st.st_mode)) {
        char real[4096];
        if (realpath(path, real) && !strcmp(real, "/"))
            return warn("it is dangerous to operate recursively on '/'");
        if (!recursive && !dirs)
            return warn("cannot remove '%s': Is a directory", path);
    }

    if (interactive) {
        fprintf(stderr, "%s: remove %s '%s'? ", prog_name,
                S_ISDIR(st.st_mode) ? "directory" : "regular file", path);
        if (!ask_yes())
            return 0;
    }

    if (S_ISDIR(st.st_mode) && recursive)
        return remove_tree(path, verbose);

    if (unlink(path) < 0)
        return warn("cannot remove '%s': %s", path, strerror(errno));
    if (verbose)
        printf(S_ISDIR(st.st_mode) ? "removed directory '%s'\n" : "removed '%s'\n", path);
    return 0;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "rRfidv")) != -1) {
        switch (c) {
        case 'r': case 'R':
            recursive = 1;
            break;
        case 'f':
            force = 1;
            interactive = 0;
            break;
        case 'i':
            interactive = 1;
            force = 0;
            break;
        case 'd':
            dirs = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            fprintf(stderr, "usage: rm [-rRfidv] file...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        if (force)
            return 0;
        warn("missing operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++)
        status |= remove_one(argv[i]);
    return status;
}
