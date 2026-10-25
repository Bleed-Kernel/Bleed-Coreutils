#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <common.h>

static const struct {
    const char *name;
    int         num;
} sigs[] = {
    { "HUP", SIGHUP }, { "INT", SIGINT }, { "QUIT", SIGQUIT }, { "ILL", SIGILL },
    { "TRAP", SIGTRAP }, { "ABRT", SIGABRT }, { "BUS", SIGBUS }, { "FPE", SIGFPE },
    { "KILL", SIGKILL }, { "USR1", SIGUSR1 }, { "SEGV", SIGSEGV }, { "USR2", SIGUSR2 },
    { "PIPE", SIGPIPE }, { "ALRM", SIGALRM }, { "TERM", SIGTERM }, { "CHLD", SIGCHLD },
    { "CONT", SIGCONT }, { "STOP", SIGSTOP }, { "TSTP", SIGTSTP }, { "TTIN", SIGTTIN },
    { "TTOU", SIGTTOU }, { "WINCH", SIGWINCH },
};

#define NSIGS (int)(sizeof(sigs) / sizeof(sigs[0]))

static int sig_from_name(const char *s) {
    if (isdigit((unsigned char)*s))
        return atoi(s);
    if (!strncasecmp(s, "SIG", 3))
        s += 3;
    for (int i = 0; i < NSIGS; i++) {
        if (!strcasecmp(s, sigs[i].name))
            return sigs[i].num;
    }
    return -1;
}

static const char *sig_name(int num) {
    for (int i = 0; i < NSIGS; i++) {
        if (sigs[i].num == num)
            return sigs[i].name;
    }
    return NULL;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int sig = SIGTERM;
    int i = 1;

    if (i < argc && (!strcmp(argv[i], "-l") || !strcmp(argv[i], "-L"))) {
        // kill -l 9 gives KILL, kill -l on its own lists them all
        if (i + 1 < argc) {
            int n = atoi(argv[i + 1]);
            const char *name = sig_name(n > 128 ? n - 128 : n);
            if (!name)
                return warn("unknown signal '%s'", argv[i + 1]);
            puts(name);
            return 0;
        }
        for (int s = 0; s < NSIGS; s++)
            printf("%2d) SIG%-8s%s", sigs[s].num, sigs[s].name, (s % 4 == 3 || s == NSIGS - 1) ? "\n" : " ");
        return 0;
    }

    if (i < argc && !strcmp(argv[i], "-s")) {
        if (i + 1 >= argc)
            return warn("option requires an argument -- 's'");
        sig = sig_from_name(argv[i + 1]);
        i += 2;
    } else if (i < argc && argv[i][0] == '-' && argv[i][1] && strcmp(argv[i], "--")) {
        sig = sig_from_name(argv[i] + 1);
        i++;
    }
    if (i < argc && !strcmp(argv[i], "--"))
        i++;

    if (sig < 0 || sig >= NSIG)
        return warn("invalid signal");
    if (i >= argc) {
        fprintf(stderr, "usage: kill [-s sig | -sig] pid...\n       kill -l [sig]\n");
        return 1;
    }

    int status = 0;
    for (; i < argc; i++) {
        char *end;
        long pid = strtol(argv[i], &end, 10);
        if (*end || end == argv[i]) {
            status = warn("(%s) - arguments must be process ids", argv[i]);
            continue;
        }
        if (kill((pid_t)pid, sig) < 0)
            status = warn("(%ld) - %s", pid, strerror(errno));
    }
    return status;
}
