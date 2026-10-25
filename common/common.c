#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

const char *prog_name = "coreutils";

void warn_init(const char *argv0) {
    if (argv0 && *argv0)
        prog_name = path_base(argv0);
}

int warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", prog_name);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    return 1;
}

const char *path_base(const char *path) {
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/')
        len--;
    while (len > 0 && path[len - 1] != '/')
        len--;
    return path + len;
}

int path_join(char *out, size_t size, const char *dir, const char *name) {
    size_t dlen = strlen(dir);
    while (dlen > 1 && dir[dlen - 1] == '/')
        dlen--;

    // path_base can hand us "name/"
    size_t nlen = strlen(name);
    while (nlen > 1 && name[nlen - 1] == '/')
        nlen--;

    int n;
    if (dlen == 1 && dir[0] == '/')
        n = snprintf(out, size, "/%.*s", (int)nlen, name);
    else
        n = snprintf(out, size, "%.*s/%.*s", (int)dlen, dir, (int)nlen, name);

    if (n < 0 || (size_t)n >= size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

int is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int write_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (w == 0) {
            errno = ENOSPC;
            return -1;
        }
        p   += w;
        len -= (size_t)w;
    }
    return 0;
}

int copy_fd(int in, int out) {
    static char buf[COPY_BUF_SIZE];

    for (;;) {
        ssize_t r = read(in, buf, sizeof(buf));
        if (r == 0)
            return 0;
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (write_all(out, buf, (size_t)r) < 0)
            return -1;
    }
}

static int name_cmp(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

char **list_dir(const char *path, int *count) {
    DIR *d = opendir(path);
    if (!d)
        return NULL;

    int n = 0, cap = 16;
    char **list = malloc(sizeof(char *) * cap);
    if (!list) {
        closedir(d);
        return NULL;
    }

    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        if (n == cap) {
            cap *= 2;
            char **grown = realloc(list, sizeof(char *) * cap);
            if (!grown)
                break;
            list = grown;
        }
        list[n] = strdup(ent->d_name);
        if (list[n])
            n++;
    }
    closedir(d);

    qsort(list, n, sizeof(char *), name_cmp);
    *count = n;
    return list;
}

void free_list(char **list, int count) {
    if (!list)
        return;
    for (int i = 0; i < count; i++)
        free(list[i]);
    free(list);
}

void human_size(char *out, size_t size, unsigned long long n) {
    static const char units[] = "BKMGTPE";
    int u = 0;
    double v = (double)n;

    while (v >= 1024.0 && u < 6) {
        v /= 1024.0;
        u++;
    }

    if (u == 0)
        snprintf(out, size, "%llu", n);
    else if (v < 10.0)
        snprintf(out, size, "%.1f%c", v, units[u]);
    else
        snprintf(out, size, "%.0f%c", v, units[u]);
}

int ask_yes(void) {
    char line[64];
    if (!fgets(line, sizeof(line), stdin))
        return 0;
    return line[0] == 'y' || line[0] == 'Y';
}

FILE *open_input(const char *path) {
    if (!strcmp(path, "-"))
        return stdin;
    FILE *f = fopen(path, "r");
    if (!f)
        warn("%s: %s", path, strerror(errno));
    return f;
}

void close_input(FILE *f) {
    if (f && f != stdin)
        fclose(f);
}
