#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int copy_tree(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0)
        return warn("cannot stat '%s': %s", src, strerror(errno));

    if (!S_ISDIR(st.st_mode)) {
        int in = open(src, O_RDONLY);
        if (in < 0)
            return warn("cannot open '%s' for reading: %s", src, strerror(errno));

        int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
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
        return status;
    }

    if (!is_dir(dst) && mkdir(dst, 0755) < 0)
        return warn("cannot create directory '%s': %s", dst, strerror(errno));

    int count = 0;
    char **names = list_dir(src, &count);
    if (!names)
        return warn("cannot read directory '%s': %s", src, strerror(errno));

    int status = 0;
    char s[PATH_MAX], d[PATH_MAX];
    for (int i = 0; i < count && !status; i++) {
        if (path_join(s, sizeof(s), src, names[i]) < 0 || path_join(d, sizeof(d), dst, names[i]) < 0) {
            status = warn("'%s/%s': %s", src, names[i], strerror(ENAMETOOLONG));
            break;
        }
        status = copy_tree(s, d);
    }

    free_list(names, count);
    return status;
}

int remove_tree(const char *path, int verbose) {
    struct stat st;
    if (stat(path, &st) < 0)
        return warn("cannot remove '%s': %s", path, strerror(errno));

    int status = 0;
    if (S_ISDIR(st.st_mode)) {
        int count = 0;
        char **names = list_dir(path, &count);
        if (!names)
            return warn("cannot read directory '%s': %s", path, strerror(errno));

        char child[PATH_MAX];
        for (int i = 0; i < count; i++) {
            if (path_join(child, sizeof(child), path, names[i]) < 0) {
                status = warn("'%s/%s': %s", path, names[i], strerror(ENAMETOOLONG));
                continue;
            }
            status |= remove_tree(child, verbose);
        }
        free_list(names, count);
        if (status)
            return status;
    }

    if (unlink(path) < 0)
        return warn("cannot remove '%s': %s", path, strerror(errno));

    if (verbose)
        printf(S_ISDIR(st.st_mode) ? "removed directory '%s'\n" : "removed '%s'\n", path);
    return 0;
}

int path_inside(const char *src, const char *dst) {
    char rs[PATH_MAX], rd[PATH_MAX], parent[PATH_MAX], full[PATH_MAX];
    if (!realpath(src, rs))
        return 0;

    // dst usually doesnt exist yet, resolve its parent instead
    snprintf(parent, sizeof(parent), "%s", dst);
    const char *base = path_base(dst);
    size_t plen = (size_t)(base - dst);
    while (plen > 1 && parent[plen - 1] == '/')
        plen--;
    if (plen == 0)
        strcpy(parent, ".");
    else
        parent[plen] = '\0';

    if (!realpath(parent, rd) || path_join(full, sizeof(full), rd, base) < 0)
        return 0;

    size_t len = strlen(rs);
    if (len == 1)
        return 1;
    return strncmp(rs, full, len) == 0 && (full[len] == '/' || full[len] == '\0');
}
