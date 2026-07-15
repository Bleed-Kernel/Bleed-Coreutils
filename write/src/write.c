#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/file.h>
#include <stdlib.h>
#include <syscalls/write.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

int get_permission(char perm[8]){
    if (strcmp(perm, "rw") == 0){
        return O_RDONLY;
    }else if (strcmp(perm, "wr") == 0){
        return O_WRONLY;
    }else if (strcmp(perm, "rdwr") == 0){
        return O_RDWR;
    }else if (strcmp(perm, "mode") == 0){
        return O_MODE;
    }else if (strcmp(perm, "creat") == 0){
        return O_CREAT;
    }else if (strcmp(perm, "trunc") == 0){
        return O_TRUNC;
    }else if (strcmp(perm, "append") == 0){
        return O_APPEND;
    }else{
        return -1;
    }

    // holy this is bad, oh well.
}

//quick and dirty program for writing directly to file descriptors or devices.
int main(int argc, const char** argv){
    if (argc != 4){
        printf("Bad Argument Count (expected write <value> <destination> <permission[rd, wr, rdwr, mode, creat, trunc, append]>)\n");
        _exit(1);
    }
    
    char permission[8];
    strncpy(permission, argv[3], sizeof(permission));

    int permission_level = get_permission(permission);
    if (permission_level == -1){
        printf("Bad Permission Level, either rd, wr, rdwr, mode, creat, trunc or append\n");
    }

    char filepath[4096];
    strncpy(filepath, argv[2], sizeof(filepath));

    char value[4096];
    strncpy(value, argv[1], sizeof(value));
    
    int fd = (int)_open(filepath, permission_level);
    if (fd < 0){
        printf("File or device %s not found!", filepath);
        _exit(1);
    }

    _write(fd, value, sizeof(value));
    _close(fd);
}
