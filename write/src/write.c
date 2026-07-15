#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/file.h>
#include <stdlib.h>
#include <syscalls/write.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

//quick and dirty program for writing directly to file descriptors or devices.
// theres also like no point freeing any of these heap allocations the program lives
// for like a few milliseconds, the kernel will handle it (tm).
int main(int argc, const char** argv){
    if (argc != 3){
        printf("Bad Argument Count (expected write <value> <destination>)\n");
        _exit(1);
    }
    
    char filepath[4096];
    strncpy(filepath, argv[2], sizeof(filepath));

    char value[4096];
    strncpy(value, argv[1], sizeof(value));

    int fd = (int)_open(filepath, O_RDWR);
    if (fd < 0){
        printf("File or device %s not found!", filepath);
        _exit(1);
    }

    _write(fd, value, sizeof(value));
    _close(fd);
}
