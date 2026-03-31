#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define BUF_SIZE 4096
#define LINE_MAX_SIZE 4096

typedef struct {
    int ignore_case;
    int invert_match;
    int print_line_number;
    int debug;
} options_t;

static int match_line(const char *line, const char *pattern, int ignore_case) {
    if (!ignore_case) {
        return strstr(line, pattern) != NULL;
    }

    size_t line_len = strlen(line);
    size_t pat_len = strlen(pattern);

    for (size_t i = 0; i + pat_len <= line_len; i++) {
        size_t j = 0;
        for (; j < pat_len; j++) {
            if (tolower((unsigned char)line[i + j]) !=
                tolower((unsigned char)pattern[j])) {
                break;
            }
        }
        if (j == pat_len) return 1;
    }

    return 0;
}

static void process_fd(int fd, const char *filename, const char *pattern, options_t *opt) {
    char buf[BUF_SIZE];
    char line[LINE_MAX_SIZE];
    size_t line_len = 0;
    ssize_t r;
    unsigned long line_num = 0;

    if (opt->debug) {
        fprintf(stderr, "[DEBUG] Processing %s\n", filename ? filename : "stdin");
    }

    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        if (opt->debug) {
            fprintf(stderr, "[DEBUG] read %zd bytes\n", r);
        }

        for (ssize_t i = 0; i < r; i++) {
            char c = buf[i];

            if (line_len < LINE_MAX_SIZE - 1) {
                line[line_len++] = c;
            }

            if (c == '\n') {
                line[line_len] = '\0';
                line_num++;

                if (opt->debug) {
                    fprintf(stderr, "[DEBUG] Line %lu: %s", line_num, line);
                }

                int matched = match_line(line, pattern, opt->ignore_case);

                if (opt->invert_match) {
                    matched = !matched;
                }

                if (opt->debug) {
                    fprintf(stderr, "[DEBUG] matched=%d\n", matched);
                }

                if (matched) {
                    if (filename) {
                        printf("%s:", filename);
                    }

                    if (opt->print_line_number) {
                        printf("%lu:", line_num);
                    }

                    printf("%s", line);
                }

                line_len = 0;
            }
        }
    }

    // Handle last line if file doesn't end with newline
    if (line_len > 0) {
        line[line_len] = '\0';
        line_num++;

        if (opt->debug) {
            fprintf(stderr, "[DEBUG] Final Line %lu: %s\n", line_num, line);
        }

        int matched = match_line(line, pattern, opt->ignore_case);

        if (opt->invert_match) {
            matched = !matched;
        }

        if (matched) {
            if (filename) {
                printf("%s:", filename);
            }

            if (opt->print_line_number) {
                printf("%lu:", line_num);
            }

            printf("%s\n", line);
        }
    }

    if (r < 0) {
        fprintf(stderr, "grep: read error: %s\n", strerror(errno));
    }
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-i] [-n] [-v] [-d] pattern [file...]\n", prog);
}

int main(int argc, char *argv[]) {
    options_t opt = {0};

    int i = 1;

    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "-i") == 0) {
            opt.ignore_case = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            opt.print_line_number = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            opt.invert_match = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            opt.debug = 1;
        } else {
            usage(argv[0]);
            return 1;
        }
        i++;
    }

    if (i >= argc) {
        usage(argv[0]);
        return 1;
    }

    const char *pattern = argv[i++];

    if (opt.debug) {
        fprintf(stderr, "[DEBUG] pattern: %s\n", pattern);
    }

    // stdin if theres no files
    
    if (i == argc) {
        process_fd(STDIN_FILENO, NULL, pattern, &opt);
        return 0;
    }

    for (; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-") == 0) {
            process_fd(STDIN_FILENO, NULL, pattern, &opt);
            continue;
        }

        if (arg[0] == '<' && arg[1] != '\0') {
            arg++;
        }

        char path[PATH_MAX];
        size_t len = strlen(arg);

        if (len >= PATH_MAX) {
            fprintf(stderr, "grep: path too long: %s\n", arg);
            continue;
        }

        memcpy(path, arg, len + 1);

        if (opt.debug) {
            fprintf(stderr, "[DEBUG] opening file: %s\n", path);
        }

        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "grep: cannot open %s: %s\n", path, strerror(errno));
            continue;
        }

        process_fd(fd, path, pattern, &opt);
        close(fd);
    }

    return 0;
}