#pragma once

#include <stddef.h>
#include <sys/stat.h>

#define COPY_BUF_SIZE 65536

// prints "prog: message" to stderr and returns 1 so callers can do status = warn(...)
int warn(const char *fmt, ...);

// last path component, "a/b/" gives "b/"
const char *path_base(const char *path);

// dir + "/" + name, -1 if it doesnt fit
int path_join(char *out, size_t size, const char *dir, const char *name);

int is_dir(const char *path);

// write everything or fail
int write_all(int fd, const void *buf, size_t len);

// copies until EOF, -1 with errno set on failure
int copy_fd(int in, int out);

// names in a directory without . and .., sorted. free with free_list
char **list_dir(const char *path, int *count);
void free_list(char **list, int count);

// 1.5K, 20M etc like ls -h
void human_size(char *out, size_t size, unsigned long long n);

// reads a line from stdin and says if it started with y
int ask_yes(void);

// name the tool was started as, set by warn_init
extern const char *prog_name;
void warn_init(const char *argv0);

// plain recursive copy for mv across filesystems, prints its own errors, 0 on success
int copy_tree(const char *src, const char *dst);

// rm -r, children are listed up front because readdir indexes shift as things get deleted
int remove_tree(const char *path, int verbose);

// is dst src or somewhere under it, cp -r a a/b would never stop
int path_inside(const char *src, const char *dst);

// opens a file argument, "-" is stdin. prints the error itself and gives NULL
#include <stdio.h>
FILE *open_input(const char *path);
void close_input(FILE *f);
