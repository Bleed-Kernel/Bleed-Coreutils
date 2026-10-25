#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int summarize;
static int human;
static int all_files;
static int bytes;
static int max_depth = -1;
static int status;

static void print_size(unsigned long long size, const char *path) {
    char buf[32];
    if (human)
        human_size(buf, sizeof(buf), size);
    else if (bytes)
        snprintf(buf, sizeof(buf), "%llu", size);
    else
        snprintf(buf, sizeof(buf), "%llu", (size + 1023) / 1024);
    printf("%s\t%s\n", buf, path);
}

static unsigned long long walk(const char *path, int depth) {
    struct stat st;
    if (stat(path, &st) < 0) {
        status = warn("cannot access '%s': %s", path, strerror(errno));
        return 0;
    }

    unsigned long long total = S_ISDIR(st.st_mode) ? 0 : (unsigned long long)st.st_size;
    int show = !summarize && (max_depth < 0 || depth <= max_depth);

    if (!S_ISDIR(st.st_mode)) {
        if (show && all_files && depth > 0)
            print_size(total, path);
        return total;
    }

    int count = 0;
    char **names = list_dir(path, &count);
    if (!names) {
        status = warn("cannot read directory '%s': %s", path, strerror(errno));
        return 0;
    }

    char child[PATH_MAX];
    for (int i = 0; i < count; i++) {
        if (path_join(child, sizeof(child), path, names[i]) == 0)
            total += walk(child, depth + 1);
    }
    free_list(names, count);

    if (show && depth > 0)
        print_size(total, path);
    return total;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int grand = 0;

    int c;
    while ((c = getopt(argc, argv, "shabcd:kx")) != -1) {
        switch (c) {
        case 's': summarize = 1; break;
        case 'h': human = 1; break;
        case 'a': all_files = 1; break;
        case 'b': bytes = 1; break;
        case 'c': grand = 1; break;
        case 'd': max_depth = atoi(optarg); break;
        case 'k': case 'x': break;
        default:
            fprintf(stderr, "usage: du [-shabck] [-d depth] [file...]\n");
            return 1;
        }
    }

    char *dot[] = { ".", NULL };
    char **args = argv + optind;
    int nargs = argc - optind;
    if (nargs == 0) {
        args = dot;
        nargs = 1;
    }

    unsigned long long sum = 0;
    for (int i = 0; i < nargs; i++) {
        unsigned long long size = walk(args[i], 0);
        print_size(size, args[i]);
        sum += size;
    }
    if (grand)
        print_size(sum, "total");
    return status;
}
