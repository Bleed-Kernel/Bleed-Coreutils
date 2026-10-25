#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int force;
static int interactive;
static int no_clobber;
static int verbose;

static int move(const char *src, const char *dst) {
    struct stat sst, dst_st;
    if (stat(src, &sst) < 0)
        return warn("cannot stat '%s': %s", src, strerror(errno));

    int exists = stat(dst, &dst_st) == 0;
    if (exists) {
        if (sst.st_dev == dst_st.st_dev && sst.st_ino == dst_st.st_ino)
            return warn("'%s' and '%s' are the same file", src, dst);
        if (S_ISDIR(dst_st.st_mode) && !S_ISDIR(sst.st_mode))
            return warn("cannot overwrite directory '%s' with non-directory", dst);
        if (!S_ISDIR(dst_st.st_mode) && S_ISDIR(sst.st_mode))
            return warn("cannot overwrite non-directory '%s' with directory '%s'", dst, src);
        if (no_clobber)
            return 0;
        if (interactive && !force) {
            fprintf(stderr, "%s: overwrite '%s'? ", prog_name, dst);
            if (!ask_yes())
                return 0;
        }
    }

    if (S_ISDIR(sst.st_mode) && path_inside(src, dst))
        return warn("cannot move '%s' to a subdirectory of itself, '%s'", src, dst);

    if (rename(src, dst) == 0)
        goto done;

    // the filesystems wont rename over something, get it out of the way and try again
    if (errno == EEXIST && exists) {
        if (unlink(dst) < 0)
            return warn("cannot move '%s' to '%s': %s", src, dst, strerror(errno));
        exists = 0;
        if (rename(src, dst) == 0)
            goto done;
    }

    // renames only work inside one directory, everything else is copy then delete like linux does across filesystems
    if (errno != EXDEV && errno != ENOSYS)
        return warn("cannot move '%s' to '%s': %s", src, dst, strerror(errno));

    if (exists && !S_ISDIR(dst_st.st_mode))
        unlink(dst);

    if (copy_tree(src, dst))
        return 1;
    if (remove_tree(src, 0))
        return 1;

done:
    if (verbose)
        printf("renamed '%s' -> '%s'\n", src, dst);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "usage: mv [-finv] source dest\n"
                    "       mv [-finv] source... directory\n");
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "finvT")) != -1) {
        switch (c) {
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
        case 'T':
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
            status |= move(argv[i], dest);
            continue;
        }

        char target[PATH_MAX];
        if (path_join(target, sizeof(target), dest, path_base(argv[i])) < 0) {
            status = warn("'%s': %s", argv[i], strerror(errno));
            continue;
        }
        status |= move(argv[i], target);
    }

    return status;
}
