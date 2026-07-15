#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/file.h>
#include <syscalls/write.h>
#include <syscalls/open.h>
#include <syscalls/close.h>

//quick and dirty program for writing directly to file descriptors or devices.
int main(int argc, const char** argv){
    if (argc != 3){
        printf("Bad Argument Count (expected write <value> <destination>)\n");
    }

    int fd = (int)_open(argv[1], O_RDWR);

    if (fd < 0){
        printf("File or device not found!");
    }

    _write(fd, argv[1], sizeof(argv[1]));
    _close(fd);
}
