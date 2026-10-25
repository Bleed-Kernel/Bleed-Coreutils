#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <common.h>

static char **base_argv;
static int base_argc;
static int max_args;
static const char *replace;
static int nul_sep;
static int trace;
static int no_empty;
static int status;

// next token from stdin, blanks split them unless quoted, -0 or -I changes that
static char *next_token(void) {
    static char *buf;
    static size_t cap;
    size_t n = 0;
    int c, quote = 0, got = 0;

    if (!buf) {
        cap = 256;
        buf = malloc(cap);
    }

    for (;;) {
        c = getchar();
        if (c == EOF)
            break;

        if (nul_sep) {
            if (c == '\0')
                break;
        } else if (replace) {
            if (c == '\n')
                break;
        } else if (quote) {
            if (c == quote) {
                quote = 0;
                continue;
            }
        } else if (c == '\'' || c == '"') {
            quote = c;
            got = 1;
            continue;
        } else if (c == '\\') {
            c = getchar();
            if (c == EOF)
                break;
        } else if (c == ' ' || c == '\t' || c == '\n') {
            if (got)
                break;
            continue;
        }

        if (n + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[n++] = (char)c;
        got = 1;
    }

    if (!got && n == 0)
        return NULL;
    buf[n] = '\0';
    return strdup(buf);
}

static void run(char **extra, int nextra) {
    int argc = base_argc + (replace ? 0 : nextra);
    char **argv = calloc(argc + 1, sizeof(char *));
    char *made[256];
    int nmade = 0;

    for (int i = 0; i < base_argc; i++) {
        argv[i] = base_argv[i];
        if (!replace || nextra == 0)
            continue;
        const char *hit = strstr(base_argv[i], replace);
        if (!hit || nmade == 256)
            continue;
        size_t rlen = strlen(replace);
        size_t len = strlen(base_argv[i]) - rlen + strlen(extra[0]) + 1;
        char *s = malloc(len);
        snprintf(s, len, "%.*s%s%s", (int)(hit - base_argv[i]), base_argv[i], extra[0], hit + rlen);
        argv[i] = made[nmade++] = s;
    }
    if (!replace) {
        for (int i = 0; i < nextra; i++)
            argv[base_argc + i] = extra[i];
    }

    if (trace) {
        for (int i = 0; i < argc; i++)
            fprintf(stderr, "%s%s", i ? " " : "", argv[i]);
        fputc('\n', stderr);
    }

    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "xargs: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    if (pid < 0) {
        status = warn("fork: %s", strerror(errno));
    } else {
        int st = 0;
        waitpid(pid, &st, 0);
        if (!WIFEXITED(st) || WEXITSTATUS(st) != 0)
            status = WIFEXITED(st) && WEXITSTATUS(st) == 127 ? 127 : 123;
    }

    for (int i = 0; i < nmade; i++)
        free(made[i]);
    free(argv);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int c;
    while ((c = getopt(argc, argv, "n:I:0trL:")) != -1) {
        switch (c) {
        case 'n': case 'L': max_args = atoi(optarg); break;
        case 'I': replace = optarg; max_args = 1; break;
        case '0': nul_sep = 1; break;
        case 't': trace = 1; break;
        case 'r': no_empty = 1; break;
        default:
            fprintf(stderr, "usage: xargs [-0rt] [-n max] [-I replace] [command [args...]]\n");
            return 1;
        }
    }

    static char *echo_argv[] = { "echo", NULL };
    base_argv = argv + optind;
    base_argc = argc - optind;
    if (base_argc == 0) {
        base_argv = echo_argv;
        base_argc = 1;
    }

    int cap = 64, n = 0, ran = 0;
    char **items = malloc(cap * sizeof(char *));
    char *tok;
    while ((tok = next_token())) {
        if (n == cap) {
            cap *= 2;
            items = realloc(items, cap * sizeof(char *));
        }
        items[n++] = tok;
        if (max_args > 0 && n >= max_args) {
            run(items, n);
            ran = 1;
            for (int i = 0; i < n; i++)
                free(items[i]);
            n = 0;
        }
    }

    if (n > 0 || (!ran && !no_empty && !replace))
        run(items, n);
    for (int i = 0; i < n; i++)
        free(items[i]);
    free(items);
    return status;
}
