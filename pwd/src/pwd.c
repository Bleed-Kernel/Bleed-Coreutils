#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <common.h>

int main(int argc, char **argv) {
    warn_init(argv[0]);
    (void)argc;

    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof(buf)))
        return warn("%s", strerror(errno));
    puts(buf);
    return 0;
}
