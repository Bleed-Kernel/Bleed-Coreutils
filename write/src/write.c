#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/file.h>
#include <stdlib.h>
#include <syscalls/write.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

#define EXPECTED_ARGS   4

int get_permission(char perm[64]){
    int flags = 0;
    int matched_any = 0;

    char work[64];
    strncpy(work, perm, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *token = strtok(work, ",");
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

//quick and dirty program for writing directly to file descriptors or devices.
int main(int argc, const char** argv){
    if (argc != EXPECTED_ARGS){
        printf("Bad Argument Count, expected %i, got %i (write <value> <destination> <permission[rd,wr,rdwr,mode,creat,trunc,append]>)\n", EXPECTED_ARGS, argc);
        _exit(1);
    }

    char permission[64];
    strncpy(permission, argv[3], sizeof(permission) - 1);
    permission[sizeof(permission) - 1] = '\0';

    int permission_level = get_permission(permission);
    if (permission_level == -1){
        printf("Bad Permission Level, comma-separated combination of: rd, wr, rdwr, mode, creat, trunc, append\n");
        _exit(1);
    }

    char filepath[4096];
    strncpy(filepath, argv[2], sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';

    char value[4096];
    strncpy(value, argv[1], sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';

    int fd = (int)_open(filepath, permission_level);
    if (fd < 0){
        printf("File or device %s not found or could not be opened (fd=%d)!\n", filepath, fd);
        _exit(1);
    }

    size_t value_len = strlen(value);
    ssize_t written = _write(fd, value, value_len);
    if (written < 0){
        printf("Write failed with error %ld\n", (long)written);
        _close(fd);
        _exit(1);
    }else if ((size_t)written != value_len){
        printf("Partial write: wrote %ld of %zu bytes\n", (long)written, value_len);
    }

    int close_result = _close(fd);
    if (close_result < 0){
        printf("Close failed with error %d\n", close_result);
        _exit(1);
    }

    return 0;
}