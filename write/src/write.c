#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syscalls.h>

#define EXPECTED_ARGS   4
#define STDIN_FD        0
#define STREAM_BUF_SIZE 4096

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

static long write_all(int fd, const char *buf, size_t len){
    size_t total = 0;
    while (total < len){
        long w = _write(fd, buf + total, (long)(len - total));
        if (w < 0) return w;
        if (w == 0) break;
        total += (size_t)w;
    }
    return (long)total;
}

//copy stdin into fd until EOF so pipes can move data of any size
static int stream_stdin(int fd){
    char buf[STREAM_BUF_SIZE];
    for (;;){
        long r = _read(STDIN_FD, buf, sizeof(buf));
        if (r == 0) return 0;
        if (r < 0){
            printf("Read from stdin failed with error %ld\n", r);
            return -1;
        }

        long w = write_all(fd, buf, (size_t)r);
        if (w < 0){
            printf("Write failed with error %ld\n", w);
            return -1;
        }
        if (w != r){
            printf("Partial write: wrote %ld of %ld bytes\n", w, r);
            return -1;
        }
    }
}

//quick and dirty program for writing directly to file descriptors or devices.
int main(int argc, const char** argv){
    if (argc != EXPECTED_ARGS){
        printf("Bad Argument Count, expected %i, got %i (write <destination> <value|-> <permission[rd,wr,rdwr,mode,creat,trunc,append]>, - reads the value from stdin)\n", EXPECTED_ARGS, argc);
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

    const char *filepath = argv[1];
    int fd = (int)_open(filepath, permission_level);
    if (fd < 0){
        printf("File or device %s not found or could not be opened (fd=%d)!\n", filepath, fd);
        _exit(1);
    }

    if (strcmp(argv[2], "-") == 0){
        if (stream_stdin(fd) < 0){
            _close(fd);
            _exit(1);
        }
    }else{
        size_t value_len = strlen(argv[2]);
        long written = write_all(fd, argv[2], value_len);
        if (written < 0){
            printf("Write failed with error %ld\n", written);
            _close(fd);
            _exit(1);
        }else if ((size_t)written != value_len){
            printf("Partial write: wrote %ld of %zu bytes\n", written, value_len);
        }
    }

    int close_result = _close(fd);
    if (close_result < 0){
        printf("Close failed with error %d\n", close_result);
        _exit(1);
    }

    return 0;
}