#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

static void print_base(const char *path, const char *suffix) {
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s", path_base(path));

    size_t len = strlen(buf);
    while (len > 1 && buf[len - 1] == '/')
        buf[--len] = '\0';

    if (suffix) {
        size_t slen = strlen(suffix);
        if (slen < len && !strcmp(buf + len - slen, suffix))
            buf[len - slen] = '\0';
    }
    puts(buf);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *suffix = NULL;
    int multiple = 0;

    int c;
    while ((c = getopt(argc, argv, "as:z")) != -1) {
        switch (c) {
        case 'a': multiple = 1; break;
        case 's': suffix = optarg; multiple = 1; break;
        case 'z': break;
        default:
            fprintf(stderr, "usage: basename name [suffix]\n"
                            "       basename [-a] [-s suffix] name...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 1;
    }

    if (!multiple) {
        print_base(argv[optind], optind + 1 < argc ? argv[optind + 1] : NULL);
        return 0;
    }
    for (int i = optind; i < argc; i++)
        print_base(argv[i], suffix);
    return 0;
}
