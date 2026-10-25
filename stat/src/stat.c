#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <common.h>

static const char *type_name(mode_t mode) {
    if (S_ISDIR(mode))  return "directory";
    if (S_ISREG(mode))  return "regular file";
    if (S_ISCHR(mode))  return "character special file";
    if (S_ISBLK(mode))  return "block special file";
    if (S_ISFIFO(mode)) return "fifo";
    if (S_ISLNK(mode))  return "symbolic link";
    return "unknown";
}

static void print_time(const char *label, time_t t) {
    char buf[64];
    struct tm *tm = localtime(&t);
    if (tm)
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    else
        strcpy(buf, "-");
    printf("%s: %s\n", label, buf);
}

// the handful of --format sequences people actually use
static void print_format(const char *fmt, const char *path, const struct stat *st) {
    for (const char *p = fmt; *p; p++) {
        if (*p == '\\' && p[1] == 'n') {
            putchar('\n');
            p++;
            continue;
        }
        if (*p != '%' || !p[1]) {
            putchar(*p);
            continue;
        }
        switch (*++p) {
        case 'n': fputs(path, stdout); break;
        case 's': printf("%lld", (long long)st->st_size); break;
        case 'F': fputs(type_name(st->st_mode), stdout); break;
        case 'a': printf("%o", st->st_mode & 07777); break;
        case 'f': printf("%x", st->st_mode); break;
        case 'i': printf("%llu", (unsigned long long)st->st_ino); break;
        case 'h': printf("%lu", (unsigned long)st->st_nlink); break;
        case 'b': printf("%ld", st->st_blocks); break;
        case 'B': printf("%ld", st->st_blksize); break;
        case 'd': printf("%llu", (unsigned long long)st->st_dev); break;
        case 'u': case 'g': putchar('0'); break;
        case 'U': case 'G': fputs("root", stdout); break;
        case 'X': printf("%lld", (long long)st->st_atime); break;
        case 'Y': printf("%lld", (long long)st->st_mtime); break;
        case 'Z': printf("%lld", (long long)st->st_ctime); break;
        case '%': putchar('%'); break;
        default:  putchar('%'); putchar(*p); break;
        }
    }
    putchar('\n');
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *format = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "--format=", 9) || !strncmp(argv[i], "--printf=", 9)) {
            format = argv[i] + 9;
            for (int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argc--;
            i--;
        }
    }

    int c;
    while ((c = getopt(argc, argv, "c:Lf")) != -1) {
        switch (c) {
        case 'c':
            format = optarg;
            break;
        case 'L': case 'f':
            break;
        default:
            fprintf(stderr, "usage: stat [-c format] file...\n");
            return 1;
        }
    }

    if (optind >= argc) {
        warn("missing operand");
        return 1;
    }

    int status = 0;
    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) < 0) {
            status = warn("cannot stat '%s': %s", argv[i], strerror(errno));
            continue;
        }

        if (format) {
            print_format(format, argv[i], &st);
            continue;
        }

        char mode[11];
        mode[0] = S_ISDIR(st.st_mode) ? 'd' : S_ISCHR(st.st_mode) ? 'c' : S_ISFIFO(st.st_mode) ? 'p' : '-';
        for (int b = 0; b < 9; b++)
            mode[b + 1] = (st.st_mode & (0400 >> b)) ? "rwxrwxrwx"[b] : '-';
        mode[10] = '\0';

        printf("  File: %s\n", argv[i]);
        printf("  Size: %-15lld Blocks: %-10ld IO Block: %-6ld %s\n",
               (long long)st.st_size, st.st_blocks, st.st_blksize, type_name(st.st_mode));
        printf("Device: %-15llu Inode: %-11llu Links: %lu\n",
               (unsigned long long)st.st_dev, (unsigned long long)st.st_ino, (unsigned long)st.st_nlink);
        printf("Access: (%04o/%s)  Uid: (    0/    root)   Gid: (    0/    root)\n", st.st_mode & 07777, mode);
        print_time("Access", st.st_atime);
        print_time("Modify", st.st_mtime);
        print_time("Change", st.st_ctime);
    }
    return status;
}
