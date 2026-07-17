#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fs/file.h>
#include <syscalls/ioctl.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

#define MIN_ARGS   4
#define MAX_ARGS   5

static int get_permission(const char* perm){
    int flags = 0;
    int matched_any = 0;

    char work[64];
    strncpy(work, perm, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char* token = strtok(work, ",");
    while (token != NULL){
        if (strcmp(token, "rd") == 0){
            flags |= O_RDONLY;
        }else if (strcmp(token, "wr") == 0){
            flags |= O_WRONLY;
        }else if (strcmp(token, "rdwr") == 0){
            flags |= O_RDWR;
        }else if (strcmp(token, "mode") == 0){
            flags |= O_MODE;
        }else if (strcmp(token, "creat") == 0){
            flags |= O_CREAT;
        }else if (strcmp(token, "trunc") == 0){
            flags |= O_TRUNC;
        }else if (strcmp(token, "append") == 0){
            flags |= O_APPEND;
        }else{
            return -1;
        }
        matched_any = 1;
        token = strtok(NULL, ",");
    }

    if (!matched_any){
        return -1;
    }

    return flags;
}

static int parse_request(const char* str, unsigned long* out){
    if (!str || *str == '\0'){
        return -1;
    }

    char* end = NULL;
    unsigned long val = strtoul(str, &end, 0);
    if (end == str || *end != '\0'){
        return -1;
    }

    *out = val;
    return 0;
}

int main(int argc, const char** argv){
    if (argc != MIN_ARGS && argc != MAX_ARGS){
        printf("Bad Argument Count, expected %i or %i, got %i (ioctl <device> <request> <arg> [permission])\n", MIN_ARGS, MAX_ARGS, argc);
        _exit(1);
    }

    int open_flags;
    if (argc == MAX_ARGS){
        open_flags = get_permission(argv[4]);
        if (open_flags == -1){
            printf("Bad Permission Level, comma-separated combination of: rd, wr, rdwr, mode, creat, trunc, append\n");
            _exit(1);
        }
    }else{
        open_flags = O_RDWR;
    }

    char filepath[4096];
    strncpy(filepath, argv[1], sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';

    unsigned long request = 0;
    if (parse_request(argv[2], &request) != 0){
        printf("Bad request code '%s', expected decimal or 0x-prefixed hex value\n", argv[2]);
        _exit(1);
    }

    char argbuf[4096];
    strncpy(argbuf, argv[3], sizeof(argbuf) - 1);
    argbuf[sizeof(argbuf) - 1] = '\0';

    void* ioctl_arg;
    char* numeric_end = NULL;
    unsigned long numeric_val = strtoul(argbuf, &numeric_end, 0);
    if (numeric_end != argbuf && *numeric_end == '\0'){
        ioctl_arg = (void*)numeric_val;
    }else{
        ioctl_arg = (void*)argbuf;
    }

    int fd = (int)_open(filepath, open_flags);
    if (fd < 0){
        printf("File or device %s not found or could not be opened (fd=%d)!\n", filepath, fd);
        _exit(1);
    }

    int result = _ioctl(fd, request, ioctl_arg);
    if (result < 0){
        printf("ioctl failed with error %d\n", result);
        _close(fd);
        _exit(1);
    }

    printf("ioctl succeeded, result=%d\n", result);

    int close_result = _close(fd);
    if (close_result < 0){
        printf("Close failed with error %d\n", close_result);
        _exit(1);
    }

    return 0;
}