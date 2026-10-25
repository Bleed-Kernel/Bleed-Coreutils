#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <common.h>

#define COLOR_DIR "\x1b[1;34m"
#define COLOR_DEV "\x1b[1;33m"
#define COLOR_END "\x1b[0m"

typedef struct {
    char        *name;
    struct stat  st;
} entry_t;

static int all;
static int almost_all;
static int long_list;
static int human;
static int one_per_line;
static int recurse;
static int dir_itself;
static int classify;
static int by_size;
static int by_time;
static int reverse;
static int color;
static int term_width = 80;

static int entry_cmp(const void *a, const void *b) {
    const entry_t *x = a, *y = b;
    int r = 0;

    if (by_size && x->st.st_size != y->st.st_size)
        r = x->st.st_size > y->st.st_size ? -1 : 1;
    else if (by_time && x->st.st_mtime != y->st.st_mtime)
        r = x->st.st_mtime > y->st.st_mtime ? -1 : 1;
    else
        r = strcmp(x->name, y->name);

    return reverse ? -r : r;
}

static void mode_string(mode_t mode, char *out) {
    out[0] = S_ISDIR(mode) ? 'd' : S_ISCHR(mode) ? 'c' : S_ISBLK(mode) ? 'b' :
             S_ISFIFO(mode) ? 'p' : S_ISLNK(mode) ? 'l' : '-';
    const char *rwx = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
        out[i + 1] = (mode & (0400 >> i)) ? rwx[i] : '-';
    out[10] = '\0';
}

static const char *color_for(mode_t mode) {
    if (!color)
        return NULL;
    if (S_ISDIR(mode))
        return COLOR_DIR;
    if (S_ISCHR(mode) || S_ISBLK(mode))
        return COLOR_DEV;
    return NULL;
}

static char type_char(mode_t mode) {
    if (S_ISDIR(mode))
        return '/';
    if (S_ISFIFO(mode))
        return '|';
    return 0;
}

static int print_name(const entry_t *e) {
    const char *c = color_for(e->st.st_mode);
    int len = (int)strlen(e->name);

    if (c)
        printf("%s%s" COLOR_END, c, e->name);
    else
        fputs(e->name, stdout);

    if (classify && type_char(e->st.st_mode)) {
        putchar(type_char(e->st.st_mode));
        len++;
    }
    return len;
}

static void print_long(entry_t *list, int n) {
    int size_w = 1;
    char buf[32];

    for (int i = 0; i < n; i++) {
        if (human)
            human_size(buf, sizeof(buf), (unsigned long long)list[i].st.st_size);
        else
            snprintf(buf, sizeof(buf), "%lld", (long long)list[i].st.st_size);
        int w = (int)strlen(buf);
        if (w > size_w)
            size_w = w;
    }

    time_t now = time(NULL);
    for (int i = 0; i < n; i++) {
        struct stat *st = &list[i].st;
        char mode[11], date[32];
        mode_string(st->st_mode, mode);

        if (human)
            human_size(buf, sizeof(buf), (unsigned long long)st->st_size);
        else
            snprintf(buf, sizeof(buf), "%lld", (long long)st->st_size);

        // older than 6 months shows the year instead of the time like coreutils
        time_t t = st->st_mtime;
        struct tm *tm = localtime(&t);
        if (tm && (now - t > 15778476 || t > now))
            strftime(date, sizeof(date), "%b %e  %Y", tm);
        else if (tm)
            strftime(date, sizeof(date), "%b %e %H:%M", tm);
        else
            strcpy(date, "?");

        printf("%s %lu root root %*s %s ", mode, (unsigned long)st->st_nlink, size_w, buf, date);
        print_name(&list[i]);
        putchar('\n');
    }
}

static void print_columns(entry_t *list, int n) {
    if (n == 0)
        return;

    int width = 0;
    for (int i = 0; i < n; i++) {
        int w = (int)strlen(list[i].name) + (classify && type_char(list[i].st.st_mode) ? 1 : 0);
        if (w > width)
            width = w;
    }
    width += 2;

    int cols = term_width / width;
    if (cols < 1)
        cols = 1;
    int rows = (n + cols - 1) / cols;

    // down the columns first like ls does
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = c * rows + r;
            if (i >= n)
                break;
            int len = print_name(&list[i]);
            if ((c + 1) * rows + r < n)
                printf("%*s", width - len, "");
        }
        putchar('\n');
    }
}

static void print_list(entry_t *list, int n) {
    qsort(list, n, sizeof(entry_t), entry_cmp);

    if (long_list) {
        print_long(list, n);
    } else if (one_per_line) {
        for (int i = 0; i < n; i++) {
            print_name(&list[i]);
            putchar('\n');
        }
    } else {
        print_columns(list, n);
    }
}

static void free_entries(entry_t *list, int n) {
    for (int i = 0; i < n; i++)
        free(list[i].name);
    free(list);
}

static int list_directory(const char *path, int show_header, int first) {
    int count = 0;
    char **names = list_dir(path, &count);
    if (!names)
        return warn("cannot open directory '%s': %s", path, strerror(errno));

    entry_t *list = calloc(count + 1, sizeof(entry_t));
    int n = 0;
    char full[PATH_MAX];

    for (int i = 0; i < count; i++) {
        if (names[i][0] == '.' && !all && !almost_all)
            continue;
        if (path_join(full, sizeof(full), path, names[i]) < 0)
            continue;

        list[n].name = strdup(names[i]);
        if (stat(full, &list[n].st) < 0)
            memset(&list[n].st, 0, sizeof(struct stat));
        n++;
    }
    free_list(names, count);

    if (show_header) {
        if (!first)
            putchar('\n');
        printf("%s:\n", path);
    }
    if (long_list) {
        unsigned long long blocks = 0;
        for (int i = 0; i < n; i++)
            blocks += ((unsigned long long)list[i].st.st_size + 1023) / 1024;
        printf("total %llu\n", blocks);
    }
    print_list(list, n);

    int status = 0;
    if (recurse) {
        for (int i = 0; i < n; i++) {
            if (!S_ISDIR(list[i].st.st_mode))
                continue;
            if (path_join(full, sizeof(full), path, list[i].name) < 0)
                continue;
            status |= list_directory(full, 1, 0);
        }
    }

    free_entries(list, n);
    return status;
}

static void usage(void) {
    fprintf(stderr, "usage: ls [-aAlh1RdFSrtC] [file...]\n");
}

int main(int argc, char **argv) {
    warn_init(argv[0]);

    int tty = isatty(STDOUT_FILENO);
    one_per_line = !tty;
    color = tty;

    struct winsize ws;
    if (tty && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;

    // --color=never and friends, nothing else long is supported
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--color", 7) != 0)
            continue;
        const char *v = argv[i][7] == '=' ? argv[i] + 8 : "always";
        color = !strcmp(v, "always") || !strcmp(v, "yes") || (!strcmp(v, "auto") && tty);
        for (int j = i; j < argc - 1; j++)
            argv[j] = argv[j + 1];
        argc--;
        i--;
    }

    int c;
    while ((c = getopt(argc, argv, "aAlh1RdFSrtCG")) != -1) {
        switch (c) {
        case 'a': all = 1; break;
        case 'A': almost_all = 1; break;
        case 'l': long_list = 1; break;
        case 'h': human = 1; break;
        case '1': one_per_line = 1; break;
        case 'C': one_per_line = 0; break;
        case 'R': recurse = 1; break;
        case 'd': dir_itself = 1; break;
        case 'F': classify = 1; break;
        case 'S': by_size = 1; by_time = 0; break;
        case 't': by_time = 1; by_size = 0; break;
        case 'r': reverse = 1; break;
        case 'G': color = 0; break;
        default:
            usage();
            return 2;
        }
    }

    char *dot[] = { ".", NULL };
    char **args = argv + optind;
    int nargs = argc - optind;
    if (nargs == 0) {
        args = dot;
        nargs = 1;
    }

    int status = 0;
    entry_t *files = calloc(nargs, sizeof(entry_t));
    int nfiles = 0;
    char **dirs = calloc(nargs, sizeof(char *));
    int ndirs = 0;

    for (int i = 0; i < nargs; i++) {
        struct stat st;
        if (stat(args[i], &st) < 0) {
            status = 2;
            warn("cannot access '%s': %s", args[i], strerror(errno));
            continue;
        }
        if (S_ISDIR(st.st_mode) && !dir_itself) {
            dirs[ndirs++] = args[i];
        } else {
            files[nfiles].name = strdup(args[i]);
            files[nfiles].st   = st;
            nfiles++;
        }
    }

    if (nfiles)
        print_list(files, nfiles);
    free_entries(files, nfiles);

    int headers = nargs > 1 || recurse;
    for (int i = 0; i < ndirs; i++) {
        if (list_directory(dirs[i], headers, i == 0 && !nfiles))
            status = status ? status : 1;
    }
    free(dirs);

    fflush(stdout);
    return status;
}
