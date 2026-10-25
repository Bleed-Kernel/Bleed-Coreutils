#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int recursive;
static int force;
static int interactive;
static int no_clobber;
static int verbose;

static int copy_path(const char *src, const char *dst);

static int same_file(const struct stat *a, const struct stat *b) {
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

static int copy_file(const char *src, const char *dst, const struct stat *sst) {
    struct stat dst_st;
    int exists = stat(dst, &dst_st) == 0;

    if (exists) {
        if (same_file(sst, &dst_st))
            return warn("'%s' and '%s' are the same file", src, dst);
        if (S_ISDIR(dst_st.st_mode))
            return warn("cannot overwrite directory '%s' with non-directory", dst);
        if (no_clobber)
            return 0;
        if (interactive) {
            fprintf(stderr, "%s: overwrite '%s'? ", prog_name, dst);
            if (!ask_yes())
                return 0;
        }
    }

    int in = open(src, O_RDONLY);
    if (in < 0)
        return warn("cannot open '%s' for reading: %s", src, strerror(errno));

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
    if (out < 0 && exists && force) {
        unlink(dst);
        out = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
    }
    if (out < 0) {
        int err = errno;
        close(in);
        return warn("cannot create regular file '%s': %s", dst, strerror(err));
    }

    int status = 0;
    if (copy_fd(in, out) < 0)
        status = warn("error copying '%s' to '%s': %s", src, dst, strerror(errno));

    close(in);
    close(out);

    if (!status && verbose)
        printf("'%s' -> '%s'\n", src, dst);
    return status;
}

static int copy_dir(const char *src, const char *dst) {
    struct stat st;
    if (stat(dst, &st) == 0) {
        if (!S_ISDIR(st.st_mode))
            return warn("cannot overwrite non-directory '%s' with directory '%s'", dst, src);
    } else {
        if (mkdir(dst, 0755) < 0)
            return warn("cannot create directory '%s': %s", dst, strerror(errno));
        if (verbose)
            printf("'%s' -> '%s'\n", src, dst);
    }

    int count = 0;
    char **names = list_dir(src, &count);
    if (!names)
        return warn("cannot read directory '%s': %s", src, strerror(errno));

    int status = 0;
    char s[PATH_MAX], d[PATH_MAX];
    for (int i = 0; i < count; i++) {
        if (path_join(s, sizeof(s), src, names[i]) < 0 || path_join(d, sizeof(d), dst, names[i]) < 0) {
            status = warn("'%s/%s': %s", src, names[i], strerror(ENAMETOOLONG));
            continue;
        }
        status |= copy_path(s, d);
    }

    free_list(names, count);
    return status;
}

static int copy_path(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0)
        return warn("cannot stat '%s': %s", src, strerror(errno));

    if (S_ISDIR(st.st_mode)) {
        if (!recursive)
            return warn("-r not specified; omitting directory '%s'", src);
        return copy_dir(src, dst);
    }
    return copy_file(src, dst, &st);
}

static int copy_one(const char *src, const char *dst) {
    if (recursive && is_dir(src) && path_inside(src, dst))
        return warn("cannot copy a directory, '%s', into itself, '%s'", src, dst);
    return copy_path(src, dst);
}

static void usage(void) {
    fprintf(stderr, "usage: cp [-rRafinvp] source dest\n"
                    "       cp [-rRafinvp] source... directory\n");
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "rRafinvpdT")) != -1) {
        switch (c) {
        case 'r': case 'R': case 'a':
            recursive = 1;
            break;
        case 'f':
            force = 1;
            interactive = 0;
            no_clobber = 0;
            break;
        case 'i':
            interactive = 1;
            no_clobber = 0;
            break;
        case 'n':
            no_clobber = 1;
            interactive = 0;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'p': case 'd': case 'T':
            break;
        default:
            usage();
            return 1;
        }
    }

    int nargs = argc - optind;
    if (nargs < 2) {
        if (nargs == 0)
            warn("missing file operand");
        else
            warn("missing destination file operand after '%s'", argv[optind]);
        usage();
        return 1;
    }

    const char *dest = argv[argc - 1];
    int dest_dir = is_dir(dest);

    if (nargs > 2 && !dest_dir)
        return warn("target '%s' is not a directory", dest);

    int status = 0;
    for (int i = optind; i < argc - 1; i++) {
        if (!dest_dir) {
            status |= copy_one(argv[i], dest);
            continue;
        }

        char target[PATH_MAX];
        if (path_join(target, sizeof(target), dest, path_base(argv[i])) < 0) {
            status = warn("'%s': %s", argv[i], strerror(errno));
            continue;
        }
        status |= copy_one(argv[i], target);
    }

    return status;
}
