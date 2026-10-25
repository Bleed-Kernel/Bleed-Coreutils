#include <ctype.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <common.h>

typedef enum {
    N_TRUE, N_NAME, N_INAME, N_PATH, N_TYPE, N_EMPTY, N_SIZE, N_PRINT, N_PRINT0,
    N_DELETE, N_EXEC, N_NOT, N_AND, N_OR
} node_kind_t;

typedef struct node {
    node_kind_t  kind;
    const char  *arg;
    char       **exec_argv;
    int          exec_argc;
    long long    size;
    int          size_cmp;
    struct node *left, *right;
} node_t;

static char **tokens;
static int ntokens, pos;
static int max_depth = -1, min_depth = 0;
static int has_action;
static int depth_first;
static int status;

static node_t *new_node(node_kind_t kind) {
    node_t *n = calloc(1, sizeof(node_t));
    n->kind = kind;
    return n;
}

static node_t *parse_or(void);

static const char *need_arg(const char *opt) {
    if (pos >= ntokens) {
        warn("missing argument to '%s'", opt);
        exit(1);
    }
    return tokens[pos++];
}

static node_t *parse_primary(void) {
    const char *t = tokens[pos++];

    if (!strcmp(t, "(")) {
        node_t *n = parse_or();
        if (pos >= ntokens || strcmp(tokens[pos], ")")) {
            warn("missing ')'");
            exit(1);
        }
        pos++;
        return n;
    }
    if (!strcmp(t, "!") || !strcmp(t, "-not")) {
        node_t *n = new_node(N_NOT);
        n->left = parse_primary();
        return n;
    }

    node_t *n = new_node(N_TRUE);
    if (!strcmp(t, "-name")) {
        n->kind = N_NAME;
        n->arg = need_arg(t);
    } else if (!strcmp(t, "-iname")) {
        n->kind = N_INAME;
        n->arg = need_arg(t);
    } else if (!strcmp(t, "-path") || !strcmp(t, "-wholename")) {
        n->kind = N_PATH;
        n->arg = need_arg(t);
    } else if (!strcmp(t, "-type")) {
        n->kind = N_TYPE;
        n->arg = need_arg(t);
    } else if (!strcmp(t, "-empty")) {
        n->kind = N_EMPTY;
    } else if (!strcmp(t, "-size")) {
        // +10k, -2M, 512c, plain numbers are 512 byte blocks like gnu find
        const char *a = need_arg(t);
        n->kind = N_SIZE;
        n->size_cmp = *a == '+' ? 1 : *a == '-' ? -1 : 0;
        if (*a == '+' || *a == '-')
            a++;
        char *end;
        n->size = strtoll(a, &end, 10);
        switch (*end) {
        case 'c': break;
        case 'k': n->size *= 1024; break;
        case 'M': n->size *= 1024 * 1024; break;
        case 'G': n->size *= 1024LL * 1024 * 1024; break;
        default:  n->size *= 512; break;
        }
    } else if (!strcmp(t, "-print")) {
        n->kind = N_PRINT;
        has_action = 1;
    } else if (!strcmp(t, "-print0")) {
        n->kind = N_PRINT0;
        has_action = 1;
    } else if (!strcmp(t, "-delete")) {
        n->kind = N_DELETE;
        has_action = 1;
        depth_first = 1;
    } else if (!strcmp(t, "-exec")) {
        n->kind = N_EXEC;
        has_action = 1;
        int start = pos;
        while (pos < ntokens && strcmp(tokens[pos], ";") && strcmp(tokens[pos], "+"))
            pos++;
        if (pos >= ntokens || pos == start) {
            warn("missing argument to '-exec'");
            exit(1);
        }
        n->exec_argv = tokens + start;
        n->exec_argc = pos - start;
        pos++;
    } else if (!strcmp(t, "-maxdepth")) {
        max_depth = atoi(need_arg(t));
    } else if (!strcmp(t, "-mindepth")) {
        min_depth = atoi(need_arg(t));
    } else if (!strcmp(t, "-depth")) {
        depth_first = 1;
    } else if (!strcmp(t, "-true")) {
    } else if (!strcmp(t, "-false")) {
        n->kind = N_NOT;
        n->left = new_node(N_TRUE);
    } else {
        warn("unknown predicate '%s'", t);
        exit(1);
    }
    return n;
}

static node_t *parse_and(void) {
    node_t *left = parse_primary();
    while (pos < ntokens && strcmp(tokens[pos], "-o") && strcmp(tokens[pos], "-or") && strcmp(tokens[pos], ")")) {
        if (!strcmp(tokens[pos], "-a") || !strcmp(tokens[pos], "-and"))
            pos++;
        node_t *n = new_node(N_AND);
        n->left = left;
        n->right = parse_primary();
        left = n;
    }
    return left;
}

static node_t *parse_or(void) {
    node_t *left = parse_and();
    while (pos < ntokens && (!strcmp(tokens[pos], "-o") || !strcmp(tokens[pos], "-or"))) {
        pos++;
        node_t *n = new_node(N_OR);
        n->left = left;
        n->right = parse_and();
        left = n;
    }
    return left;
}

static int run_exec(node_t *n, const char *path) {
    char **argv = calloc(n->exec_argc + 1, sizeof(char *));
    char *subst[64];
    int nsubst = 0;

    for (int i = 0; i < n->exec_argc; i++) {
        const char *a = n->exec_argv[i];
        const char *brace = strstr(a, "{}");
        if (!brace || nsubst == 64) {
            argv[i] = (char *)a;
            continue;
        }
        size_t len = strlen(a) - 2 + strlen(path) + 1;
        char *s = malloc(len);
        snprintf(s, len, "%.*s%s%s", (int)(brace - a), a, path, brace + 2);
        argv[i] = subst[nsubst++] = s;
    }

    fflush(stdout);
    int ok = 0;
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "find: '%s': %s\n", argv[0], strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        int st = 0;
        waitpid(pid, &st, 0);
        ok = WIFEXITED(st) && WEXITSTATUS(st) == 0;
    }

    for (int i = 0; i < nsubst; i++)
        free(subst[i]);
    free(argv);
    return ok;
}

static int eval(node_t *n, const char *path, const struct stat *st) {
    char t;
    switch (n->kind) {
    case N_TRUE:
        return 1;
    case N_NAME:
        return fnmatch(n->arg, path_base(path), 0) == 0;
    case N_INAME: {
        char a[256], b[256];
        snprintf(a, sizeof(a), "%s", n->arg);
        snprintf(b, sizeof(b), "%s", path_base(path));
        for (char *p = a; *p; p++) *p = (char)tolower((unsigned char)*p);
        for (char *p = b; *p; p++) *p = (char)tolower((unsigned char)*p);
        return fnmatch(a, b, 0) == 0;
    }
    case N_PATH:
        return fnmatch(n->arg, path, 0) == 0;
    case N_TYPE:
        t = n->arg[0];
        return (t == 'f' && S_ISREG(st->st_mode)) || (t == 'd' && S_ISDIR(st->st_mode)) ||
               (t == 'c' && S_ISCHR(st->st_mode)) || (t == 'b' && S_ISBLK(st->st_mode)) ||
               (t == 'p' && S_ISFIFO(st->st_mode)) || (t == 'l' && S_ISLNK(st->st_mode));
    case N_EMPTY:
        if (S_ISDIR(st->st_mode)) {
            int count = 0;
            char **names = list_dir(path, &count);
            free_list(names, count);
            return names && count == 0;
        }
        return S_ISREG(st->st_mode) && st->st_size == 0;
    case N_SIZE:
        if (n->size_cmp > 0)
            return st->st_size > n->size;
        if (n->size_cmp < 0)
            return st->st_size < n->size;
        return st->st_size == n->size;
    case N_PRINT:
        puts(path);
        return 1;
    case N_PRINT0:
        fputs(path, stdout);
        putchar('\0');
        return 1;
    case N_DELETE:
        if (unlink(path) < 0) {
            status = warn("cannot delete '%s': %s", path, strerror(errno));
            return 0;
        }
        return 1;
    case N_EXEC:
        return run_exec(n, path);
    case N_NOT:
        return !eval(n->left, path, st);
    case N_AND:
        return eval(n->left, path, st) && eval(n->right, path, st);
    case N_OR:
        return eval(n->left, path, st) || eval(n->right, path, st);
    }
    return 0;
}

static void visit(const char *path, node_t *expr, int depth) {
    struct stat st;
    if (stat(path, &st) < 0) {
        status = warn("'%s': %s", path, strerror(errno));
        return;
    }

    int match_here = depth >= min_depth;
    if (match_here && !depth_first && eval(expr, path, &st) && !has_action)
        puts(path);

    if (S_ISDIR(st.st_mode) && (max_depth < 0 || depth < max_depth)) {
        int count = 0;
        char **names = list_dir(path, &count);
        if (!names) {
            status = warn("'%s': %s", path, strerror(errno));
        } else {
            char child[PATH_MAX];
            for (int i = 0; i < count; i++) {
                if (path_join(child, sizeof(child), path, names[i]) == 0)
                    visit(child, expr, depth + 1);
            }
            free_list(names, count);
        }
    }

    if (match_here && depth_first && eval(expr, path, &st) && !has_action)
        puts(path);
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int first_expr = 1;
    while (first_expr < argc && argv[first_expr][0] != '-' &&
           strcmp(argv[first_expr], "(") && strcmp(argv[first_expr], "!"))
        first_expr++;

    tokens  = argv + first_expr;
    ntokens = argc - first_expr;
    node_t *expr = ntokens > 0 ? parse_or() : new_node(N_TRUE);
    if (pos < ntokens) {
        warn("unexpected '%s'", tokens[pos]);
        return 1;
    }

    if (first_expr == 1) {
        visit(".", expr, 0);
    } else {
        for (int i = 1; i < first_expr; i++)
            visit(argv[i], expr, 0);
    }
    fflush(stdout);
    return status;
}
