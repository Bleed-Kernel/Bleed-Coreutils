#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/file.h>
#include <syscalls/write.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

//quick and dirty program for writing directly to file descriptors or devices.
int main(int argc, const char** argv){
    if (argc != 3){
        printf("Bad Argument Count (expected write <value> <destination>)\n");
        _exit(1);
    }
    
    char filepath[4096];
    strncpy(filepath, argv[1], 4096);

    int fd = (int)_open(filepath, O_RDWR);
    if (fd < 0){
        printf("File or device %s not found!", filepath);
    }

    _write(fd, filepath, sizeof(argv[1]));
    _close(fd);
}
