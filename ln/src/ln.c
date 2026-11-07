#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int force;
static int no_deref;
static int verbose;

static int make_link(const char *target, const char *link) {
    struct stat st;
    if (lstat(link, &st) == 0) {
        if (!force)
            return warn("failed to create symbolic link '%s': File exists", link);
        if (S_ISDIR(st.st_mode))
            return warn("cannot overwrite directory '%s'", link);
        if (unlink(link) < 0)
            return warn("cannot remove '%s': %s", link, strerror(errno));
    }

    if (symlink(target, link) < 0)
        return warn("failed to create symbolic link '%s': %s", link, strerror(errno));
    if (verbose)
        printf("'%s' -> '%s'\n", link, target);
    return 0;
}

// a folder (or a link to one, unless -n) as the last name means the links go inside it
static int into_dir(const char *path) {
    struct stat st;
    if (no_deref && lstat(path, &st) == 0 && S_ISLNK(st.st_mode))
        return 0;
    return is_dir(path);
}

int main(int argc, char **argv) {
    prog_name = "ln";
    int symbolic = 0;

    int opt;
    while ((opt = getopt(argc, argv, "sfnvT")) != -1) {
        switch (opt) {
        case 's': symbolic = 1; break;
        case 'f': force = 1; break;
        case 'n': no_deref = 1; break;
        case 'v': verbose = 1; break;
        case 'T': no_deref = 2; break;
        default:
            fprintf(stderr, "usage: ln -s [-fnvT] TARGET LINK\n       ln -s [-fnv] TARGET... DIRECTORY\n");
            return 1;
        }
    }

    int n = argc - optind;
    if (!symbolic) {
        warn("hard links arent supported, use ln -s");
        return 1;
    }
    if (n < 1) {
        warn("missing file operand");
        return 1;
    }

    // ln -s target makes ./basename(target)
    if (n == 1)
        return make_link(argv[optind], path_base(argv[optind])) ? 1 : 0;

    const char *last = argv[argc - 1];
    if (n == 2 && (no_deref == 2 || !into_dir(last)))
        return make_link(argv[optind], last) ? 1 : 0;
    if (!into_dir(last)) {
        warn("target '%s' is not a directory", last);
        return 1;
    }

    int status = 0;
    char path[PATH_MAX];
    for (int i = optind; i < argc - 1; i++) {
        if (path_join(path, sizeof(path), last, path_base(argv[i])) < 0) {
            status = warn("'%s': %s", argv[i], strerror(ENAMETOOLONG));
            continue;
        }
        status |= make_link(argv[i], path);
    }
    return status ? 1 : 0;
}
