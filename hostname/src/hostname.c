#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "sf")) != -1) {
        if (c == '?') {
            fprintf(stderr, "usage: hostname [-s]\n");
            return 1;
        }
    }

    if (optind < argc)
        return warn("setting the hostname isnt supported, edit /initrd/etc/hostname");

    char name[256];
    if (gethostname(name, sizeof(name)) < 0)
        return warn("%s", strerror(errno));
    puts(name);
    return 0;
}
