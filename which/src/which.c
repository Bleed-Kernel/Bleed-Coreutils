#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static int executable(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int all = 0;

    int c;
    while ((c = getopt(argc, argv, "as")) != -1) {
        switch (c) {
        case 'a': all = 1; break;
        case 's': break;
        default:
            fprintf(stderr, "usage: which [-a] name...\n");
            return 1;
        }
    }

    const char *path = getenv("PATH");
    if (!path)
        path = "/initrd/bin";

    int status = 0;
    for (int i = optind; i < argc; i++) {
        if (strchr(argv[i], '/')) {
            if (executable(argv[i]))
                puts(argv[i]);
            else
                status = 1;
            continue;
        }

        int found = 0;
        const char *p = path;
        while (*p) {
            const char *colon = strchr(p, ':');
            size_t len = colon ? (size_t)(colon - p) : strlen(p);
            char dir[PATH_MAX], full[PATH_MAX];
            snprintf(dir, sizeof(dir), "%.*s", (int)len, len ? p : ".");

            if (path_join(full, sizeof(full), dir, argv[i]) == 0 && executable(full)) {
                puts(full);
                found = 1;
                if (!all)
                    break;
            }
            if (!colon)
                break;
            p = colon + 1;
        }
        if (!found)
            status = 1;
    }
    return status;
}
