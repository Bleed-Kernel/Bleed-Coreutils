#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <common.h>

enum { U_S = 1, U_N = 2, U_R = 4, U_V = 8, U_M = 16, U_P = 32, U_I = 64, U_O = 128 };

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int show = 0;

    int c;
    while ((c = getopt(argc, argv, "asnrvmpio")) != -1) {
        switch (c) {
        case 'a': show = U_S | U_N | U_R | U_V | U_M | U_O; break;
        case 's': show |= U_S; break;
        case 'n': show |= U_N; break;
        case 'r': show |= U_R; break;
        case 'v': show |= U_V; break;
        case 'm': show |= U_M; break;
        case 'p': show |= U_P; break;
        case 'i': show |= U_I; break;
        case 'o': show |= U_O; break;
        default:
            fprintf(stderr, "usage: uname [-asnrvmpio]\n");
            return 1;
        }
    }
    if (!show)
        show = U_S;

    struct utsname u;
    if (uname(&u) < 0)
        return warn("cannot get system name");

    const char *parts[8] = { u.sysname, u.nodename, u.release, u.version, u.machine, u.machine, u.machine, "Bleed" };
    int first = 1;
    for (int i = 0; i < 8; i++) {
        if (!(show & (1 << i)))
            continue;
        printf("%s%s", first ? "" : " ", parts[i]);
        first = 0;
    }
    putchar('\n');
    return 0;
}
