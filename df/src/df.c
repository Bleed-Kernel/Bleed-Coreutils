#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bleed/syscalls.h>
#include <common.h>

#define MAX_MOUNTS 64

static int human;
static int show_type;

static void fmt_size(char *out, size_t size, unsigned long long bytes) {
    if (human)
        human_size(out, size, bytes);
    else
        snprintf(out, size, "%llu", bytes / 1024);
}

static int covers(const char *mount, const char *path) {
    size_t len = strlen(mount);
    if (!strcmp(mount, "/"))
        return 1;
    return !strncmp(path, mount, len) && (path[len] == '\0' || path[len] == '/') ? (int)len : 0;
}

static void print_mount(const user_mount_info_t *m) {
    unsigned long long total = m->blocks_total * m->block_size;
    unsigned long long avail = m->blocks_free * m->block_size;
    unsigned long long used  = total - avail;
    char t[32], u[32], a[32];
    fmt_size(t, sizeof(t), total);
    fmt_size(u, sizeof(u), used);
    fmt_size(a, sizeof(a), avail);

    int pct = total ? (int)((used * 100 + total - 1) / total) : 0;
    const char *dev = m->device[0] ? m->device : m->fs;

    if (show_type)
        printf("%-16s %-8s %10s %10s %10s %4d%% %s\n", dev, m->fs, t, u, a, pct, m->path);
    else
        printf("%-16s %10s %10s %10s %4d%% %s\n", dev, t, u, a, pct, m->path);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "hTkaP")) != -1) {
        switch (c) {
        case 'h': human = 1; break;
        case 'T': show_type = 1; break;
        case 'k': case 'a': case 'P': break;
        default:
            fprintf(stderr, "usage: df [-hT] [file...]\n");
            return 1;
        }
    }

    user_mount_info_t mounts[MAX_MOUNTS];
    int n = 0;
    for (int i = 0; i < MAX_MOUNTS && _mountinfo(i, &mounts[n]) == 0; i++)
        n++;

    const char *size_hdr = human ? "Size" : "1K-blocks";
    if (show_type)
        printf("%-16s %-8s %10s %10s %10s %5s %s\n", "Filesystem", "Type", size_hdr, "Used", "Avail", "Use%", "Mounted on");
    else
        printf("%-16s %10s %10s %10s %5s %s\n", "Filesystem", size_hdr, "Used", "Avail", "Use%", "Mounted on");

    if (optind >= argc) {
        for (int i = 0; i < n; i++)
            print_mount(&mounts[i]);
        return 0;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        char full[4096];
        if (!realpath(argv[i], full)) {
            status = warn("%s: %s", argv[i], strerror(errno));
            continue;
        }

        int best = -1, best_len = 0;
        for (int m = 0; m < n; m++) {
            int len = covers(mounts[m].path, full);
            if (len > best_len) {
                best_len = len;
                best = m;
            }
        }
        if (best >= 0)
            print_mount(&mounts[best]);
    }
    return status;
}
